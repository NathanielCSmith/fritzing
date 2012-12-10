/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision$:
$Author$:
$Date$

********************************************************************/


// with lots of suggestions from http://cc.ee.ntu.edu.tw/~ywchang/Courses/PD/unit6.pdf
// and some help from http://workbench.lafayette.edu/~nestorj/cadapplets/MazeApplet/src/
// more (unused) suggestions at http://embedded.eecs.berkeley.edu/Alumni/pinhong/ee244/4-routing.PDF

// TODO:
//
//      schematic view
//          use a different cost function: cost is manhattandistance + penalty for direction changes
//          netlabels are the equivalent of jumpers. Use successive numbers for the labels.
//
//      remove terminal ids just once at make master time?
//
//      net reordering/rip-up-and-reroute
//          is there a better way than move back by one?
//
//      keepout dialog
//
//      raster back to vector
//          curve-fitting? use a bezier?  http://kobus.ca/seminars/ugrad/NM5_curve_s02.pdf
//
//      many examples too close to the border
//          shift-register-2x, weckerino
//
//      put drawlines in extendborder
//     
//      when converting to vector, must check that diagonals are clear
//          handle this at traceback time? 
//              set some flags for neighbors being clear and keep the flagged grid points as part of the trace?
//
//      dynamic cost function based on distance to any target point?
//
//      via/jumper placement must ensure minimum distance from source
//          jumper placement must be away from vias
//
//      check clean up is really clearing pointers
//
//      crash after 25 or so rounds with amadeus bridge (no longer there, but do see a crash at various times)
//
//      when multiple traces connect to one via, traces tend to connect to each other instead of the via
//          see shift register example
//
//      figure out what is taking so long once the router is creating traces
//
//      huge memory leak during routing: what's going on?
//
//      allow multiple routes to reach GridTarget--eliminate all queued GridPoints with greater cost
//      
//

#include "mazerouter.h"
#include "../../sketch/pcbsketchwidget.h"
#include "../../debugdialog.h"
#include "../../items/virtualwire.h"
#include "../../items/tracewire.h"
#include "../../items/jumperitem.h"
#include "../../items/via.h"
#include "../../items/resizableboard.h"
#include "../../utils/graphicsutils.h"
#include "../../utils/graphutils.h"
#include "../../utils/textutils.h"
#include "../../utils/folderutils.h"
#include "../../connectors/connectoritem.h"
#include "../../items/moduleidnames.h"
#include "../../processeventblocker.h"
#include "../../svg/groundplanegenerator.h"
#include "../../svg/svgfilesplitter.h"
#include "../../fsvgrenderer.h"
#include "../drc.h"
#include "../../connectors/svgidlayer.h"

#include <QApplication>
#include <QMessageBox> 
#include <QSettings>
#include <QCryptographicHash>

#include <qmath.h>
#include <limits>

//////////////////////////////////////

static const int MaximumProgress = 1000;

static QString CancelledMessage;

static const int DefaultMaxCycles = 10;

static const quint32 GridObstacle = 0xffffffff;
static const quint32 GridSource = 0xfffffffe;
static const quint32 GridTarget = 0xfffffffd;
static const quint32 GridIllegal = 0xfffffffc;

static const uint Layer1Cost = 100;
static const uint CrossLayerCost = 100;
static const uint ViaCost = 1000;

static const uchar GridPointDone = 1;
static const uchar GridPointNorth = 2;
static const uchar GridPointEast = 4;
static const uchar GridPointSouth = 8;
static const uchar GridPointWest = 16;

static const uchar JumperStart = 1;
static const uchar JumperEnd = 2;

static const double MinTraceManhattanLength = 0.1;  // pixels

////////////////////////////////////////////////////////////////////

bool blocked(GridPoint & gp, int dx1, int dy1, int dx2, int dy2) {
    Q_UNUSED(dy1);
    uchar flag;
    if (dx1 == 0) {
        flag = (dx2 == 1) ? GridPointEast : GridPointWest;
    }
    else {
        flag = (dy2 == 1) ? GridPointSouth : GridPointNorth;
    }
    return ((gp.flags & flag) == 0);
}

bool atLeast(const QPointF & p1, const QPointF & p2) {
    return (qAbs(p1.x() - p2.x()) >= MinTraceManhattanLength) || (qAbs(p1.y() - p2.y()) >= MinTraceManhattanLength);
}

void printOrder(const QString & msg, QList<int> & order) {
    QString string(msg);
    foreach (int i, order) {
        string += " " + QString::number(i);
    }
    DebugDialog::debug(string);
}

QString getPartID(const QDomElement & element) {
    QString partID = element.attribute("partID");
    if (!partID.isEmpty()) return partID;

    QDomNode parent = element.parentNode();
    if (parent.isNull()) return "";

    return getPartID(parent.toElement());
}

bool idsMatch(const QDomElement & element, QMultiHash<QString, QString> & partIDs) {
    QString partID = getPartID(element);
    QStringList svgIDs = partIDs.values(partID);
    if (svgIDs.count() == 0) return false;

    QDomElement tempElement = element;
    while (tempElement.attribute("partID").isEmpty()) {
        QString id = tempElement.attribute("id");
        if (!id.isEmpty() && svgIDs.contains(id)) return true;

        tempElement = tempElement.parentNode().toElement();
    }

    return false;
}

bool byPinsWithin(Net * n1, Net * n2)
{
	if (n1->pinsWithin < n2->pinsWithin) return true;
    if (n1->pinsWithin > n2->pinsWithin) return false;

    return n1->net->count() <= n2->net->count();
}

bool byOrder(Trace & t1, Trace & t2) {
    return (t1.order < t2.order);
}

/* 
inline double initialCost(QPoint p1, QPoint p2) {
    //return qAbs(p1.x() - p2.x()) + qAbs(p1.y() - p2.y());
    return qSqrt(GraphicsUtils::distanceSqd(p1, p2));
}
*/

inline double distanceCost(const QPoint & p1, const QPoint & p2) {
    return GraphicsUtils::distanceSqd(p1, p2);
}

inline double manhattanCost(const QPoint & p1, const QPoint & p2) {
    return qMax(qAbs(p1.x() - p2.x()), qAbs(p1.y() - p2.y()));
}

////////////////////////////////////////////////////////////////////

bool GridPoint::operator<(const GridPoint& other) const {
    // make sure lower cost is first
    return cost > other.cost;
}

GridPoint::GridPoint(QPoint p, int zed) {
    z = zed;
    x = p.x();
    y = p.y();
    flags = 0;
}

GridPoint::GridPoint() 
{
    flags = 0;
}

////////////////////////////////////////////////////////////////////

Grid::Grid(int sx, int sy, int sz) {
    x = sx;
    y = sy;
    z = sz;

    data = (quint32 *) calloc(x * y * z, 4);   // calloc initializes grid to 0
}

uint Grid::at(int sx, int sy, int sz) {
    return *(data + (sz * y * x) + (sy * x) + sx);
}

void Grid::setAt(int sx, int sy, int sz, uint value) {
   *(data + (sz * y * x) + (sy * x) + sx) = value;
}

QList<QPoint> Grid::init(int sx, int sy, int sz, int width, int height, const QImage & image, quint32 value, bool collectPoints) {
    QList<QPoint> points;
    const uchar * bits1 = image.constScanLine(0);
    int bytesPerLine = image.bytesPerLine();
	for (int iy = sy; iy < sy + height; iy++) {
        int offset = iy * bytesPerLine;
		for (int ix = sx; ix < sx + width; ix++) {
            int byteOffset = (ix >> 3) + offset;
            uchar mask = DRC::BitTable[ix & 7];

            if (*(bits1 + byteOffset) & mask) continue;

            setAt(ix, iy, sz, value);
            if (collectPoints) {
                points.append(QPoint(ix, iy));
            }
			//DebugDialog::debug(QString("p1:%1 p2:%2").arg(p1, 0, 16).arg(p2, 0, 16));
		}
	}

    return points;
}


QList<QPoint> Grid::init4(int sx, int sy, int sz, int width, int height, const QImage & image, quint32 value, bool collectPoints) {
    // pixels are 4 x 4 bits
    QList<QPoint> points;
    const uchar * bits1 = image.constScanLine(0);
    int bytesPerLine = image.bytesPerLine();
	for (int iy = sy; iy < sy + height; iy++) {
        int offset = iy * bytesPerLine * 4;
		for (int ix = sx; ix < sx + width; ix++) {
            int byteOffset = (ix >> 1) + offset;
            uchar mask = ix & 1 ? 0x0f : 0xf0;

            if ((*(bits1 + byteOffset) & mask) != mask) ;
            else if ((*(bits1 + byteOffset + bytesPerLine) & mask) != mask) ;
            else if ((*(bits1 + byteOffset + bytesPerLine + bytesPerLine) & mask) != mask) ;
            else if ((*(bits1 + byteOffset + bytesPerLine + bytesPerLine + bytesPerLine) & mask) != mask) ;
            else continue;  // "pixel" is all white

            setAt(ix, iy, sz, value);
            if (collectPoints) {
                points.append(QPoint(ix, iy));
            }
			//DebugDialog::debug(QString("p1:%1 p2:%2").arg(p1, 0, 16).arg(p2, 0, 16));
		}
	}

    return points;
}

////////////////////////////////////////////////////////////////////


Score::Score() {
	totalRoutedCount = totalViaCount = 0;
    anyUnrouted = false;
    reorderNet = -1;
}

void Score::setOrdering(const NetOrdering & _ordering) {
    reorderNet = -1;
    if (ordering.order.count() > 0) {
        bool remove = false;
        for (int i = 0; i < ordering.order.count(); i++) {
            if (!remove && (ordering.order.at(i) == _ordering.order.at(i))) continue;

            remove = true;
            int netIndex = ordering.order.at(i);
            traces.remove(netIndex);
            int c = routedCount.value(netIndex);
            routedCount.remove(netIndex);
            totalRoutedCount -= c;
            c = viaCount.value(netIndex);
            viaCount.remove(netIndex);
            totalViaCount -= c;
        }
    }
    ordering = _ordering;
    printOrder("new  ", ordering.order);
}

////////////////////////////////////////////////////////////////////

MazeRouter::MazeRouter(PCBSketchWidget * sketchWidget, QGraphicsItem * board, bool adjustIf) : Autorouter(sketchWidget)
{
    m_displayItem[0] = m_displayItem[1] = NULL;
    m_displayImage[0] = m_displayImage[1] = NULL;

    CancelledMessage = tr("Autorouter was cancelled.");

	QSettings settings;
	m_maxCycles = settings.value("cmrouter/maxcycles", DefaultMaxCycles).toInt();
		
	m_bothSidesNow = sketchWidget->routeBothSides();
	m_board = board;
    m_temporaryBoard = false;

	if (m_board) {
		m_maxRect = m_board->sceneBoundingRect();
	}
	else {
        QRectF itemsBoundingRect;
	    foreach(QGraphicsItem *item,  m_sketchWidget->scene()->items()) {
		    if (!item->isVisible()) continue;

            itemsBoundingRect |= item->sceneBoundingRect();
	    }
		m_maxRect = itemsBoundingRect;  // itemsBoundingRect is not reliable.  m_sketchWidget->scene()->itemsBoundingRect();
		if (adjustIf) {
            m_maxRect.adjust(-m_maxRect.width() / 2, -m_maxRect.height() / 2, m_maxRect.width() / 2, m_maxRect.height() / 2);
		}
        m_board = new QGraphicsRectItem(m_maxRect);
        m_board->setVisible(false);
        m_sketchWidget->scene()->addItem(m_board);
        m_temporaryBoard = true;
	}

    m_standardWireWidth = m_sketchWidget->getAutorouterTraceWidth();

    /*
    // for debugging leave the last result hanging around
    QList<QGraphicsPixmapItem *> pixmapItems;
    foreach (QGraphicsItem * item, m_sketchWidget->scene()->items()) {
        QGraphicsPixmapItem * pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
        if (pixmapItem) pixmapItems << pixmapItem;
    }
    foreach (QGraphicsPixmapItem * pixmapItem, pixmapItems) {
        delete pixmapItem;
    }
    */

	ViewGeometry vg;
	vg.setWireFlags(m_sketchWidget->getTraceFlag());
	ViewLayer::ViewLayerID bottom = sketchWidget->getWireViewLayerID(vg, ViewLayer::Bottom);
	m_viewLayerIDs << bottom;
	if  (m_bothSidesNow) {
		ViewLayer::ViewLayerID top = sketchWidget->getWireViewLayerID(vg, ViewLayer::Top);
		m_viewLayerIDs.append(top);
	}
}

MazeRouter::~MazeRouter()
{
    foreach (QDomDocument * doc, m_masterDocs) {
        delete doc;
    }
    if (m_displayItem[0]) {
        delete m_displayItem[0];
    }
    if (m_displayItem[1]) {
        delete m_displayItem[1];
    }
    if (m_displayImage[0]) {
        delete m_displayImage[0];
    }
    if (m_displayImage[1]) {
        delete m_displayImage[1];
    }
    if (m_temporaryBoard && m_board != NULL) {
        delete m_board;
    }
}

void MazeRouter::start()
{	
    bool isPCBType = m_sketchWidget->autorouteTypePCB();
	if (isPCBType && m_board == NULL) {
		QMessageBox::warning(NULL, QObject::tr("Fritzing"), QObject::tr("Cannot autoroute: no board (or multiple boards) found"));
		return;
	}

    if (isPCBType) m_costFunction = distanceCost;
    else m_costFunction = manhattanCost;

	m_maximumProgressPart = 1;
	m_currentProgressPart = 0;
	m_keepoutPixels = m_sketchWidget->getKeepout();			// 15 mils space (in pixels)
    m_keepoutMils = m_keepoutPixels * GraphicsUtils::StandardFritzingDPI / GraphicsUtils::SVGDPI;
    m_keepoutGrid = m_keepoutPixels / m_standardWireWidth;

    double ringThickness, holeSize;
	m_sketchWidget->getViaSize(ringThickness, holeSize);
	int gridViaSize = qCeil((ringThickness + ringThickness + holeSize + m_keepoutPixels + m_keepoutPixels) / m_standardWireWidth);
    m_halfGridViaSize = gridViaSize / 2;

    QSizeF jumperSize = m_sketchWidget->jumperItemSize();
    int gridJumperSize = qCeil((qMax(jumperSize.width(), jumperSize.height()) + m_keepoutPixels  + m_keepoutPixels) / m_standardWireWidth);
    m_halfGridJumperSize = gridJumperSize / 2;

	emit setMaximumProgress(MaximumProgress);
	emit setProgressMessage("");
	emit setCycleMessage("round 1 of:");
	emit setCycleCount(m_maxCycles);

	m_sketchWidget->ensureTraceLayersVisible();

	QHash<ConnectorItem *, int> indexer;
	m_sketchWidget->collectAllNets(indexer, m_allPartConnectorItems, false, m_bothSidesNow);

    removeOffBoard(isPCBType, true);

	if (m_allPartConnectorItems.count() == 0) {
        QString message = isPCBType ?  QObject::tr("No connections (on the PCB) to route.") : QObject::tr("No connections to route.");
		QMessageBox::information(NULL, QObject::tr("Fritzing"), message);
		Autorouter::cleanUpNets();
		return;
	}

	QUndoCommand * parentCommand = new QUndoCommand("Autoroute");
	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::UndoOnly, parentCommand);
	initUndo(parentCommand);

	QVector<int> netCounters(m_allPartConnectorItems.count());
    NetList netList;
    int totalToRoute = 0;
	for (int i = 0; i < m_allPartConnectorItems.count(); i++) {
		netCounters[i] = (m_allPartConnectorItems[i]->count() - 1) * 2;			// since we use two connectors at a time on a net
        Net * net = new Net;
        net->net = m_allPartConnectorItems[i];

        QList<ConnectorItem *> todo;
        todo.append(*(net->net));
        while (todo.count() > 0) {
            ConnectorItem * first = todo.takeFirst();
            QList<ConnectorItem *> equi;
            equi.append(first);
	        ConnectorItem::collectEqualPotential(equi, m_bothSidesNow, (ViewGeometry::RatsnestFlag | ViewGeometry::NormalFlag | ViewGeometry::PCBTraceFlag | ViewGeometry::SchematicTraceFlag) ^ m_sketchWidget->getTraceFlag());
            foreach (ConnectorItem * equ, equi) {
                todo.removeOne(equ);
            }
            net->subnets.append(equi);
        }

        if (net->subnets.count() < 2) {
            // net is already routed
            continue;
        }

        net->pinsWithin = findPinsWithin(net->net);
        netList.nets << net;
        totalToRoute += net->net->count() - 1;
	}

    qSort(netList.nets.begin(), netList.nets.end(), byPinsWithin);
    NetOrdering initialOrdering;
    int ix = 0;
    foreach (Net * net, netList.nets) {
        // id is the same as the order in netList
        initialOrdering.order << ix;
        net->id = ix++;
    }

	if (m_bothSidesNow) {
		emit wantBothVisible();
	}

	ProcessEventBlocker::processEvents(); // to keep the app  from freezing
	if (m_cancelled || m_stopTracing) {
		restoreOriginalState(parentCommand);
		cleanUpNets(netList);
		return;
	}

    QSizeF gridSize(m_maxRect.width() / m_standardWireWidth, m_maxRect.height() / m_standardWireWidth);   
    QSize boardImageSize(qCeil(gridSize.width()), qCeil(gridSize.height())); 

    QImage * boardImage = NULL;
    if (m_board && !m_temporaryBoard) {
        boardImage = new QImage(boardImageSize.width() * 4, boardImageSize.height() * 4, QImage::Format_Mono);
        boardImage->fill(0);
        makeBoard(boardImage, m_keepoutGrid, gridSize);   
    }

	ProcessEventBlocker::processEvents(); // to keep the app  from freezing
	if (m_cancelled || m_stopTracing) {
		restoreOriginalState(parentCommand);
		cleanUpNets(netList);
		return;
	}

    m_displayImage[0] = new QImage(boardImageSize, QImage::Format_ARGB32);
    m_displayImage[0]->fill(0);
    m_displayImage[1] = new QImage(boardImageSize, QImage::Format_ARGB32);
    m_displayImage[1]->fill(0);

    QString message;
    bool gotMasters = makeMasters(message);
	if (m_cancelled || m_stopTracing || !gotMasters) {
		restoreOriginalState(parentCommand);
		cleanUpNets(netList);
		return;
	}

	QList<NetOrdering> allOrderings;
    allOrderings << initialOrdering;
    Score bestScore;
    Score currentScore;
    int run = 0;
	for (; run < m_maxCycles && run < allOrderings.count(); run++) {
		QString msg= tr("best so far: %1 of %2 routed").arg(bestScore.totalRoutedCount).arg(totalToRoute);
		if (m_sketchWidget->usesJumperItem()) {
			msg +=  tr(" with %n vias", "", bestScore.totalViaCount);
		}
		emit setProgressMessage(msg);
		emit setCycleMessage(tr("round %1 of:").arg(run + 1));
		ProcessEventBlocker::processEvents();
        currentScore.setOrdering(allOrderings.at(run));
        currentScore.anyUnrouted = false;
		routeNets(netList, false, currentScore, boardImage, gridSize, allOrderings);
		if (bestScore.ordering.order.count() == 0) {
            bestScore = currentScore;
        }
        else {
            if (currentScore.totalRoutedCount > bestScore.totalRoutedCount || m_stopTracing) {
                bestScore = currentScore;
            }
            else if (currentScore.totalRoutedCount == bestScore.totalRoutedCount && currentScore.totalViaCount < bestScore.totalViaCount) {
                bestScore = currentScore;
            }
        }
		if (m_cancelled || bestScore.anyUnrouted == false || m_stopTracing) break;
	}

    emit disableButtons();

	//DebugDialog::debug("done running");


	if (m_cancelled) {
		doCancel(parentCommand);
		return;
	}

    if (m_stopTracing) {
        emit setProgressMessage(tr("Routing stopped! Now cleaning up..."));
    }
    else if (!bestScore.anyUnrouted) {
        emit setProgressMessage(tr("Routing complete! Now cleaning up..."));
    }
    else {
		emit setCycleMessage(tr("round %1 of:").arg(run));
        emit setProgressMessage(tr("Routing reached round %1. Now cleaning up...").arg(m_maxCycles));
        routeNets(netList, true, bestScore, boardImage, gridSize, allOrderings);
    }    
	ProcessEventBlocker::processEvents();

    createTraces(netList, bestScore, parentCommand);

	cleanUpNets(netList);

	new CleanUpWiresCommand(m_sketchWidget, CleanUpWiresCommand::RedoOnly, parentCommand);

	m_sketchWidget->pushCommand(parentCommand);
	m_sketchWidget->repaint();
	DebugDialog::debug("\n\n\nautorouting complete\n\n\n");

	if (m_offBoardConnectors.count() > 0) {
		QSet<ItemBase *> parts;
		foreach (ConnectorItem * connectorItem, m_offBoardConnectors) {
			parts.insert(connectorItem->attachedTo()->layerKinChief());
		}
        QMessageBox::information(NULL, QObject::tr("Fritzing"), tr("Note: the autorouter did not route %n parts, because they are not located entirely on the board.", "", parts.count()));
	}
}

void MazeRouter::updateProgress(int num, int denom) 
{
	emit setProgressValue((int) MaximumProgress * (m_currentProgressPart + (num / (double) denom)) / (double) m_maximumProgressPart);
}

int MazeRouter::findPinsWithin(QList<ConnectorItem *> * net) {
    int count = 0;
    QRectF r;
    foreach (ConnectorItem * connectorItem, *net) {
        r |= connectorItem->sceneBoundingRect();
    }

    foreach (QGraphicsItem * item, m_sketchWidget->scene()->items(r)) {
        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
        if (connectorItem == NULL) continue;

        if (net->contains(connectorItem)) continue;

        count++;
    }

    return count;
}

bool MazeRouter::makeBoard(QImage * boardImage, double keepoutGrid, const QSizeF gridSize) {
	LayerList viewLayerIDs;
	viewLayerIDs << ViewLayer::Board;
	QRectF boardImageRect;
	bool empty;
	QString boardSvg = m_sketchWidget->renderToSVG(GraphicsUtils::SVGDPI, viewLayerIDs, true, boardImageRect, m_board, GraphicsUtils::StandardFritzingDPI, false, false, empty);
	if (boardSvg.isEmpty()) {
		return false;
	}

    QByteArray boardByteArray;
    QString tempColor("#ffffff");
    QStringList exceptions;
	exceptions << "none" << "";
    if (!SvgFileSplitter::changeColors(boardSvg, tempColor, exceptions, boardByteArray)) {
		return false;
	}

	QSvgRenderer renderer(boardByteArray);
	QPainter painter;
	painter.begin(boardImage);
	painter.setRenderHint(QPainter::Antialiasing, false);
    QRectF r(QPointF(0, 0), gridSize);
	renderer.render(&painter /*, r */);
	painter.end();

    // board should be white, borders should be black

#ifndef QT_NO_DEBUG
	boardImage->save(FolderUtils::getUserDataStorePath("") + "/mazeMakeBoard.png");
#endif

    // extend it given that the board image is * 4
    DRC::extendBorder(keepoutGrid * 4, boardImage);

#ifndef QT_NO_DEBUG
	boardImage->save(FolderUtils::getUserDataStorePath("") + "/mazeMakeBoard2.png");
#endif

    return true;
}

bool MazeRouter::makeMasters(QString & message) {
    QList<ViewLayer::ViewLayerSpec> layerSpecs;
    layerSpecs << ViewLayer::Bottom;
    if (m_bothSidesNow) layerSpecs << ViewLayer::Top;

    foreach (ViewLayer::ViewLayerSpec viewLayerSpec, layerSpecs) {  
	    LayerList viewLayerIDs = m_sketchWidget->routingLayers(viewLayerSpec);
        QRectF masterImageRect;
        bool empty;
	    QString master = m_sketchWidget->renderToSVG(GraphicsUtils::SVGDPI, viewLayerIDs, true, masterImageRect, m_board, GraphicsUtils::StandardFritzingDPI, false, false, empty);
        if (master.isEmpty()) {
            continue;
	    }

	    QDomDocument * masterDoc = new QDomDocument();
        m_masterDocs.insert(viewLayerSpec, masterDoc);

	    QString errorStr;
	    int errorLine;
	    int errorColumn;
	    if (!masterDoc->setContent(master, &errorStr, &errorLine, &errorColumn)) {
            message = tr("Unexpected SVG rendering failure--contact fritzing.org");
		    return false;
	    }

	    ProcessEventBlocker::processEvents();
        if (m_cancelled) {
            message = CancelledMessage;
            return false;
        }

        QDomElement root = masterDoc->documentElement();
        SvgFileSplitter::forceStrokeWidth(root, 2 * m_keepoutMils, "#000000", true, true);
        QString forDebugging = masterDoc->toByteArray();
        DebugDialog::debug("master " + forDebugging);
    }

    return true;
}

bool MazeRouter::routeNets(NetList & netList, bool makeJumper, Score & currentScore, QImage * boardImage, const QSizeF gridSize, QList<NetOrdering> & allOrderings)
{
    RouteThing routeThing;
    routeThing.r = QRectF(QPointF(0, 0), gridSize);
    routeThing.layerSpecs << ViewLayer::Bottom;
    if (m_bothSidesNow) routeThing.layerSpecs << ViewLayer::Top;
    routeThing.ikeepout = qCeil(m_keepoutGrid);
    routeThing.makeJumper = false;

    bool result = true;

    initTraceDisplay();
    bool previousTraces = false;
    foreach (int netIndex, currentScore.ordering.order) {
        if (m_cancelled || m_stopTracing) {
            return false;
        }

        Net * net = netList.nets.at(netIndex);
        if (currentScore.traces.values(netIndex).count() == net->net->count() - 1) {
            // traces were generated in a previous run
            foreach (Trace trace, currentScore.traces.values(netIndex)) {
                drawTrace(trace);
            }
            previousTraces = true;
            continue;
        }

        if (previousTraces) {
            updateDisplay(0);
            if (m_bothSidesNow) updateDisplay(1);
        }

        if (currentScore.routedCount.value(netIndex) > 0) {
            // should only happen when makeJumpers = true
            currentScore.totalRoutedCount -= currentScore.routedCount.value(netIndex);
            currentScore.routedCount.insert(netIndex, 0);
            currentScore.totalViaCount -= currentScore.viaCount.value(netIndex);
            currentScore.viaCount.insert(netIndex, 0);
            currentScore.traces.remove(netIndex);    
        }

        //foreach (ConnectorItem * connectorItem, *(net->net)) {
        //    if (connectorItem->attachedTo()->layerKinChief()->id() == 12407630) {
        //        connectorItem->debugInfo("what");
        //        break;
        //    }
        //}

        QList< QList<ConnectorItem *> > subnets;
        foreach (QList<ConnectorItem *> subnet, net->subnets) {
            QList<ConnectorItem *> copy(subnet);
            subnets.append(copy);
        }

        findNearestPair(subnets, routeThing.nearest);
        if (subnets.at(routeThing.nearest.i).count() < subnets.at(routeThing.nearest.j).count()) {
            routeThing.nearest.swap();
        }

        QPointF jp = routeThing.nearest.jc->sceneAdjustedTerminalPoint(NULL) - m_maxRect.topLeft();
        routeThing.gridTarget = QPoint(jp.x() / m_standardWireWidth, jp.y() / m_standardWireWidth);
        routeThing.grid = new Grid(qCeil(gridSize.width()), qCeil(gridSize.height()), m_bothSidesNow ? 2 : 1);

        QList<Trace> traceList = currentScore.traces.values();
        traceObstacles(traceList, netIndex, routeThing.grid, routeThing.ikeepout);
        static int oi = 0;

        foreach (ViewLayer::ViewLayerSpec viewLayerSpec, routeThing.layerSpecs) {  
            int z = viewLayerSpec == ViewLayer::Bottom ? 0 : 1;

            QList<QDomElement> alsoNetElements;
            QDomDocument * masterDoc = m_masterDocs.value(viewLayerSpec);

            //QString temp = masterDoc->toString();
            QList<QDomElement> & netElements = (z == 0 ? routeThing.netElements0 : routeThing.netElements1);
            QList<QDomElement> & notNetElements = (z == 0 ? routeThing.notNetElements0 : routeThing.notNetElements1);

            DRC::splitNetPrep(masterDoc, *(net->net), m_keepoutMils, DRC::NotNet, netElements, alsoNetElements, notNetElements, true, true);
            foreach (QDomElement element, netElements) {
                element.setTagName("g");
            }
            if (boardImage) {
                QImage obstaclesImage = boardImage->copy();
                DRC::renderOne(masterDoc, &obstaclesImage, routeThing.r);
                routeThing.grid->init4(0, 0, z, routeThing.grid->x, routeThing.grid->y, obstaclesImage, GridObstacle, false);
                #ifndef QT_NO_DEBUG
                    obstaclesImage.save(FolderUtils::getUserDataStorePath("") + QString("/obstacles%1.png").arg(oi++));
                #endif
            }
            else {
                QImage obstaclesImage(qCeil(gridSize.width()), qCeil(gridSize.height()), QImage::Format_Mono);
                obstaclesImage.fill(0xffffffff);
                DRC::renderOne(masterDoc, &obstaclesImage, routeThing.r);
	            QPainter painter;
	            painter.begin(&obstaclesImage);
	            painter.setRenderHint(QPainter::Antialiasing, false);
                QPen pen = painter.pen();
                pen.setWidth(qCeil(m_keepoutGrid));
                pen.setColor(0xff000000);
                painter.setPen(pen);
                painter.drawLine(0, 0, obstaclesImage.width() - 1, 0);
                painter.drawLine(0, obstaclesImage.height() - 1, obstaclesImage.width() - 1, obstaclesImage.height() - 1);
                painter.drawLine(0, 0, 0, obstaclesImage.height() - 1);
                painter.drawLine(obstaclesImage.width() - 1, 0, obstaclesImage.width() - 1, obstaclesImage.height() - 1);
	            painter.end();
                routeThing.grid->init(0, 0, z, routeThing.grid->x, routeThing.grid->y, obstaclesImage, GridObstacle, false);
                #ifndef QT_NO_DEBUG
                    obstaclesImage.save(FolderUtils::getUserDataStorePath("") + QString("/obstacles%1.png").arg(oi++));
                #endif
            }

            //updateDisplay(grid, z);

            prepSourceAndTarget(masterDoc, routeThing, subnets, z);
        }

        routeThing.unrouted = false;
        if (!routeOne(makeJumper, currentScore, netIndex, routeThing, allOrderings)) {
            result = false;
        }

        updateDisplay(routeThing.grid, 0);
        if (m_bothSidesNow) updateDisplay(routeThing.grid, 1);

        while (result && subnets.count() > 2) {
            /*
            DebugDialog::debug(QString("\nnearest %1 %2").arg(nearest.i).arg(nearest.j));
            nearest.ic->debugInfo("\ti");
            nearest.jc->debugInfo("\tj");
            int ix = 0;
            foreach (QList<ConnectorItem *> subnet, subnets) {
                foreach(ConnectorItem * connectorItem, subnet) {
                    connectorItem->debugInfo(QString::number(ix));
                }
                ix++;
            }
            */
            
            result = routeNext(makeJumper, routeThing, subnets, currentScore, netIndex, allOrderings);
        }

        delete routeThing.grid;

        // restore masterdoc
        foreach (QDomElement element, routeThing.netElements0) {
            SvgFileSplitter::forceStrokeWidth(element, 2 * m_keepoutMils, "#000000", false, false);
        }
        foreach (QDomElement element, routeThing.netElements1) {
            SvgFileSplitter::forceStrokeWidth(element, 2 * m_keepoutMils, "#000000", false, false);
        }

        routeThing.netElements0.clear();
        routeThing.netElements1.clear();
        routeThing.notNetElements0.clear();
        routeThing.notNetElements1.clear();
        routeThing.pq = std::priority_queue<GridPoint>();

        if (result == false) break;
    }

    return result;
}

bool MazeRouter::routeOne(bool makeJumper, Score & currentScore, int netIndex, RouteThing & routeThing, QList<NetOrdering> & allOrderings) {
    Trace newTrace;
    int viaCount;
    newTrace.gridPoints = route(routeThing, viaCount);
    if (m_cancelled || m_stopTracing) {
        return false;
    }

    if (newTrace.gridPoints.count() == 0) {
        if (makeJumper) {
            routeJumper(netIndex, routeThing, currentScore);
        }
        else {
            routeThing.unrouted = true;
            if (currentScore.reorderNet < 0) {
                for (int i = 0; i < currentScore.ordering.order.count(); i++) {
                    if (currentScore.ordering.order.at(i) == netIndex) {
                        if (moveBack(currentScore, i, allOrderings)) {
                            currentScore.reorderNet = netIndex;
                        }
                        break;
                    }
                }
            }

            currentScore.anyUnrouted = true;
            if (currentScore.reorderNet >= 0) {
                // rip up and reroute unless this net is already the first one on the list
                return false;
            }
            // unable to move the 0th net so keep going
        }
    }
    else {
        insertTrace(newTrace, netIndex, currentScore, viaCount);
        updateDisplay(0);
        if (m_bothSidesNow) updateDisplay(1);
    }

    return true;
}

void MazeRouter::routeJumper(int netIndex, RouteThing & routeThing, Score & currentScore) {
    routeThing.makeJumper = true;

    routeThing.jumperDistance = std::numeric_limits<double>::max();
    clearExpansionForJumper(routeThing.grid, GridSource, routeThing.pq);
    int viaCount;
    route(routeThing, viaCount);
    if (routeThing.jumperDistance < std::numeric_limits<double>::max()) {
        Trace sourceTrace;
        sourceTrace.flags = JumperStart;
        int sourceViaCount;
        sourceTrace.gridPoints = traceBack(routeThing.jumperLocation, routeThing.grid, sourceViaCount, GridSource);

        routeThing.jumperDistance = std::numeric_limits<double>::max();
        routeThing.gridTarget.setX(routeThing.jumperLocation.x);
        routeThing.gridTarget.setY(routeThing.jumperLocation.y);
        clearExpansionForJumper(routeThing.grid, GridTarget, routeThing.pq);
        route(routeThing, viaCount);
        if (routeThing.jumperDistance < std::numeric_limits<double>::max()) {
            Trace destTrace;
            destTrace.flags = JumperEnd;
            int targetViaCount;
            destTrace.gridPoints = traceBack(routeThing.jumperLocation, routeThing.grid, targetViaCount, GridSource);

            insertTrace(sourceTrace, netIndex, currentScore, sourceViaCount);
            insertTrace(destTrace, netIndex, currentScore, targetViaCount);
            updateDisplay(0);
            if (m_bothSidesNow) updateDisplay(1);
        }
    }

    clearExpansion(routeThing.grid);  
    routeThing.makeJumper = false;
}


bool MazeRouter::routeNext(bool makeJumper, RouteThing & routeThing, QList< QList<ConnectorItem *> > & subnets, Score & currentScore, int netIndex, QList<NetOrdering> & allOrderings) 
{
    bool result = true;

    QList<ConnectorItem *> combined;
    if (routeThing.unrouted) {
        if (routeThing.nearest.i < routeThing.nearest.j) {
            subnets.removeAt(routeThing.nearest.j);
            combined = subnets.takeAt(routeThing.nearest.i);
        }
        else {
            combined = subnets.takeAt(routeThing.nearest.i);
            subnets.removeAt(routeThing.nearest.j);
        }
    }
    else {
        combined.append(subnets.at(routeThing.nearest.i));
        combined.append(subnets.at(routeThing.nearest.j));
        if (routeThing.nearest.i < routeThing.nearest.j) {
            subnets.removeAt(routeThing.nearest.j);
            subnets.removeAt(routeThing.nearest.i);
        }
        else {
            subnets.removeAt(routeThing.nearest.i);
            subnets.removeAt(routeThing.nearest.j);
        }
    }
    subnets.prepend(combined);
    routeThing.nearest.i = 0;
    routeThing.nearest.j = -1;
    routeThing.nearest.distance = std::numeric_limits<double>::max();
    findNearestPair(subnets, 0, combined, routeThing.nearest);
    quint32 value = GridSource;
    if (subnets.at(routeThing.nearest.i).count() < subnets.at(routeThing.nearest.j).count()) {
        routeThing.nearest.swap();
        value = GridTarget;
    }

    QPointF jp = routeThing.nearest.jc->sceneAdjustedTerminalPoint(NULL) - m_maxRect.topLeft();
    routeThing.gridTarget = QPoint(jp.x() / m_standardWireWidth, jp.y() / m_standardWireWidth);

    routeThing.pq = std::priority_queue<GridPoint>();

    foreach (ViewLayer::ViewLayerSpec viewLayerSpec, routeThing.layerSpecs) {  
        int z = viewLayerSpec == ViewLayer::Bottom ? 0 : 1;
        QDomDocument * masterDoc = m_masterDocs.value(viewLayerSpec);
        prepSourceAndTarget(masterDoc, routeThing, subnets, z);
    }

    // redraw traces from this net
    foreach (Trace trace, currentScore.traces.values(netIndex)) {
        if (value == GridTarget) {
            foreach (GridPoint gridPoint, trace.gridPoints) {
                routeThing.grid->setAt(gridPoint.x, gridPoint.y, gridPoint.z, GridTarget);
            }
        }
        else {
            foreach (GridPoint gridPoint, trace.gridPoints) {
                routeThing.grid->setAt(gridPoint.x, gridPoint.y, gridPoint.z, GridSource);
                int crossLayerCost = 0;
                if (m_bothSidesNow) {
                    if (routeThing.nearest.jc->attachedToViewLayerID() == ViewLayer::Copper0 && gridPoint.z == 1) {
                        crossLayerCost = Layer1Cost;
                    }
                    else if (routeThing.nearest.ic->attachedToViewLayerID() == ViewLayer::Copper1 && gridPoint.z == 0) {
                        crossLayerCost = Layer1Cost;
                    }
                }

                gridPoint.cost = /* initialCost(QPoint(gridPoint.x, gridPoint.y), routeThing.gridTarget) + */ crossLayerCost;
                gridPoint.flags = 0;
                routeThing.pq.push(gridPoint);                  
            }
        }
    }

    //updateDisplay(grid, 0);
    //if (m_bothSidesNow) updateDisplay(grid, 1);

    routeThing.unrouted = false;
    result = routeOne(makeJumper, currentScore, netIndex, routeThing, allOrderings);

    return result;
}

bool MazeRouter::moveBack(Score & currentScore, int index, QList<NetOrdering> & allOrderings) {
    if (index == 0) {
        return false;  // nowhere to move back to
    }

    QList<int> order(currentScore.ordering.order);
    //printOrder("start", order);
    int netIndex = order.takeAt(index);
    //printOrder("minus", order);
    for (int i = index - 1; i >= 0; i--) {
        bool done = true;
        order.insert(i, netIndex);
        //printOrder("plus ", order);
        foreach (NetOrdering ordering, allOrderings) {
            bool gotOne = true;
            for (int j = 0; j < order.count(); j++) {
                if (order.at(j) != ordering.order.at(j)) {
                    gotOne = false;
                    break;
                }
            }
            if (gotOne) {
                done = false;
                break; 
            }
        }
        if (done == true) {
            NetOrdering newOrdering;
            newOrdering.order = order;
            allOrderings.append(newOrdering);
            //printOrder("done ", newOrdering.order);

            /*
            const static int watch[] = { 0, 1, 2, 3, 6, 12, 14, 4, 5, 7, 8, 9, 10, 11, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64 };
            // {0, 1, 2, 4, 3, 12, 6, 14, 5, 7, 8, 9, 10, 11, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
            
            bool matches = true;
            for (int i = 0; i < order.count(); i++) {
                if (order.at(i) != watch[i]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                DebugDialog::debug("order matches");
            }
            */
            return true;
        }
        order.removeAt(i);
    }

    return false;
}

void MazeRouter::prepSourceAndTarget(QDomDocument * masterDoc, RouteThing & routeThing, QList< QList<ConnectorItem *> > & subnets, int z) 
{
    QList<QDomElement> & netElements = (z == 0 ? routeThing.netElements0 : routeThing.netElements1);
    QList<QDomElement> & notNetElements = (z == 0 ? routeThing.notNetElements0 : routeThing.notNetElements1);

    foreach (QDomElement element, notNetElements) {
        element.setTagName("g");
    }

    QList<ConnectorItem *> li = subnets.at(routeThing.nearest.i);
    QList<QPoint> sourcePoints = renderSource(masterDoc, z, routeThing.grid, netElements, li, GridSource, true, routeThing.r, true);

    int crossLayerCost = 0;
    if (m_bothSidesNow) {
        if (routeThing.nearest.jc->attachedToViewLayerID() == ViewLayer::Copper0 && z == 1) {
            crossLayerCost = Layer1Cost;
        }
        else if (routeThing.nearest.jc->attachedToViewLayerID() == ViewLayer::Copper1 && z == 0) {
            crossLayerCost = Layer1Cost;
        }
    }

    foreach (QPoint p, sourcePoints) {
        GridPoint gridPoint(p, z);
        gridPoint.cost = /* initialCost(p, routeThing.gridTarget) + */ crossLayerCost;
        routeThing.pq.push(gridPoint);
    }

    QList<ConnectorItem *> lj = subnets.at(routeThing.nearest.j);
    renderSource(masterDoc, z, routeThing.grid, netElements, lj, GridTarget, true, routeThing.r, false);

    //updateDisplay(grid, z);

    // restore masterdoc (except for netElements stroke-width)
    foreach (QDomElement element, netElements) {
        element.setTagName(element.attribute("former"));
        element.removeAttribute("net");
    }
    foreach (QDomElement element, notNetElements) {
        element.setTagName(element.attribute("former"));
        element.removeAttribute("net");
    }
}

void MazeRouter::findNearestPair(QList< QList<ConnectorItem *> > & subnets, Nearest & nearest) {
    nearest.distance = std::numeric_limits<double>::max();
    nearest.i = nearest.j = -1;
    nearest.ic = nearest.jc = NULL;
    for (int i = 0; i < subnets.count() - 1; i++) {
        QList<ConnectorItem *> inet = subnets.at(i);
        findNearestPair(subnets, i, inet, nearest);
    }
}

void MazeRouter::findNearestPair(QList< QList<ConnectorItem *> > & subnets, int inetix, QList<ConnectorItem *> & inet, Nearest & nearest) {
    for (int j = inetix + 1; j < subnets.count(); j++) {
        QList<ConnectorItem *> jnet = subnets.at(j);
        foreach (ConnectorItem * ic, inet) {
            QPointF ip = ic->sceneAdjustedTerminalPoint(NULL);
            ConnectorItem * icc = ic->getCrossLayerConnectorItem();
            foreach (ConnectorItem * jc, jnet) {
                ConnectorItem * jcc = jc->getCrossLayerConnectorItem();
                if (jc == ic || jcc == ic) continue;

                QPointF jp = jc->sceneAdjustedTerminalPoint(NULL);
                double d = qSqrt(GraphicsUtils::distanceSqd(ip, jp)) / m_standardWireWidth;
                if (ic->attachedToViewLayerID() != jc->attachedToViewLayerID()) {
                    if (jcc != NULL || icc != NULL) {
                        // may not need a via
                        d += CrossLayerCost;
                    }
                    else {
                        // requires at least one via
                        d += ViaCost;
                    }
                }
                else {
                    if (jcc != NULL && icc != NULL && ic->attachedToViewLayerID() == ViewLayer::Copper1) {
                        // route on the bottom when possible
                        d += Layer1Cost;
                    }
                }
                if (d < nearest.distance) {
                    nearest.distance = d;
                    nearest.i = inetix;
                    nearest.j = j;
                    nearest.ic = ic;
                    nearest.jc = jc;
                }
            }
        }
    }
}

QList<QPoint> MazeRouter::renderSource(QDomDocument * masterDoc, int z, Grid * grid, QList<QDomElement> & netElements, QList<ConnectorItem *> & subnet, quint32 value, bool clearElements, const QRectF & r, bool collectPoints) {
    if (clearElements) {
        foreach (QDomElement element, netElements) {
            element.setTagName("g");
        }
    }

    QImage image(grid->x * 4, grid->y * 4, QImage::Format_Mono);
    image.fill(0xffffffff);
    QMultiHash<QString, QString> partIDs;
    QRectF itemsBoundingRect;
    foreach (ConnectorItem * connectorItem, subnet) {
        ItemBase * itemBase = connectorItem->attachedTo();
        SvgIdLayer * svgIdLayer = connectorItem->connector()->fullPinInfo(itemBase->viewIdentifier(), itemBase->viewLayerID());
        partIDs.insert(QString::number(itemBase->id()), svgIdLayer->m_svgId);
        itemsBoundingRect |= connectorItem->sceneBoundingRect();
    }
    foreach (QDomElement element, netElements) {
        if (idsMatch(element, partIDs)) {
            element.setTagName(element.attribute("former"));
        }
    }

    int x1 = qFloor((itemsBoundingRect.left() - m_maxRect.left()) / m_standardWireWidth);
    int y1 = qFloor((itemsBoundingRect.top() - m_maxRect.top()) / m_standardWireWidth);
    int x2 = qCeil((itemsBoundingRect.right() - m_maxRect.left()) / m_standardWireWidth);
    int y2 = qCeil((itemsBoundingRect.bottom() - m_maxRect.top()) / m_standardWireWidth);

    DRC::renderOne(masterDoc, &image, r);
#ifndef QT_NO_DEBUG
    static int rsi = 0;
	image.save(FolderUtils::getUserDataStorePath("") + QString("/rendersource%1.png").arg(rsi++));
#endif
    return grid->init4(x1, y1, z, x2 - x1, y2 - y1, image, value, collectPoints);
}

QList<GridPoint> MazeRouter::route(RouteThing & routeThing, int & viaCount)
{
    bool result = false;
    GridPoint target;
    while (!routeThing.pq.empty()) {
        GridPoint gp = routeThing.pq.top();
        routeThing.pq.pop();

        if (gp.flags & GridPointDone) {
            result = true;
            target = gp;
            break;
        }

        expand(gp, routeThing);
        if (m_cancelled || m_stopTracing) {
            break;
        }
    }


    if (!result) {
        //updateDisplay(grid, 0);
        //if (m_bothSidesNow) updateDisplay(grid, 1);
        QList<GridPoint> points;
        return points;
    }

    QList<GridPoint> points = traceBack(target, routeThing.grid, viaCount, GridSource);
    //updateDisplay(grid, 0);
    //if (m_bothSidesNow) updateDisplay(grid, 1);
    clearExpansion(routeThing.grid);  

    return points;
}

QList<GridPoint> MazeRouter::traceBack(GridPoint & gridPoint, Grid * grid, int & viaCount, quint32 destination) {
    viaCount = 0;
    QList<GridPoint> points;
    points << gridPoint;
    while (true) {
        quint32 val = grid->at(gridPoint.x, gridPoint.y, gridPoint.z);
        if (val == destination) {
            break;
        }

        // can only be one neighbor with lower value
        GridPoint next = traceBackOne(gridPoint, grid, -1, 0, 0, val);
        if (next.cost == GridIllegal) {
            next = traceBackOne(gridPoint, grid, 1, 0, 0, val);
            if (next.cost == GridIllegal) {
                next = traceBackOne(gridPoint, grid, 0, -1, 0, val);
                if (next.cost == GridIllegal) {
                    next = traceBackOne(gridPoint, grid, 0, 1, 0, val);
                    if (next.cost == GridIllegal) {
                        next = traceBackOne(gridPoint, grid, 0, 0, -1, val);
                        if (next.cost == GridIllegal) {
                            next = traceBackOne(gridPoint, grid, 0, 0, 1, val);
                            if (next.cost == GridIllegal) {
                                // traceback failed--is this possible?
                                points.clear();
                                break;      
                            }
                        }
                    }
                }
            }
        }

        //if (grid->at(next.x - 1, next.y, next.z) != GridObstacle) next.flags |= GridPointWest;
        //if (grid->at(next.x + 1, next.y, next.z) != GridObstacle) next.flags |= GridPointEast;
        //if (grid->at(next.x, next.y - 1, next.z) != GridObstacle) next.flags |= GridPointNorth;
        //if (grid->at(next.x, next.y + 1, next.z) != GridObstacle) next.flags |= GridPointSouth;
        points << next;
        if (next.z != gridPoint.z) viaCount++;
        gridPoint = next;
    }

    return points;
}

GridPoint MazeRouter::traceBackOne(GridPoint & gridPoint, Grid * grid, int dx, int dy, int dz, quint32 val) {
    GridPoint next;
    next.cost = GridIllegal;

    next.x = gridPoint.x + dx;
    if (next.x < 0 || next.x >= grid->x) {
        return next;
    }

    next.y = gridPoint.y + dy;
    if (next.y < 0 || next.y >= grid->y) {
        return next;
    }

    next.z = gridPoint.z + dz;
    if (next.z < 0 || next.z >= grid->z) {
        return next;
    }

    quint32 nextval = grid->at(next.x, next.y, next.z);
    switch (nextval) {
        case GridObstacle:
            return next;
        case GridTarget:
            return next;
        case GridSource:
            next.cost = -1;
            return next;
        case 0:
            // never got involved
            return next;
        default:
            if (nextval < val) {
                next.cost = nextval;
            }
            return next;
    }

}

void MazeRouter::expand(GridPoint & gridPoint, RouteThing & routeThing)
{
    if (gridPoint.x > 0) expandOne(gridPoint, routeThing, -1, 0, 0, false);
    if (gridPoint.x < routeThing.grid->x - 1) expandOne(gridPoint, routeThing, 1, 0, 0, false);
    if (gridPoint.y > 0) expandOne(gridPoint, routeThing, 0, -1, 0, false);
    if (gridPoint.y < routeThing.grid->y - 1) expandOne(gridPoint, routeThing, 0, 1, 0, false);
    if (gridPoint.z > 0) expandOne(gridPoint, routeThing, 0, 0, -1, true);
    if (gridPoint.z < routeThing.grid->z - 1) expandOne(gridPoint, routeThing, 0, 0, 1, true);
}

void MazeRouter::expandOne(GridPoint & gridPoint, RouteThing & routeThing, int dx, int dy, int dz, bool crossLayer) {
    GridPoint next;
    next.x = gridPoint.x + dx;
    next.y = gridPoint.y + dy;
    next.z = gridPoint.z + dz;

    quint32 nextval = routeThing.grid->at(next.x, next.y, next.z);
    switch (nextval) {
        case GridObstacle:
        case GridSource:
            return;
        case GridTarget:
            next.flags |= GridPointDone;
            break;
        case 0:
            // got a new point
            break;
        default:
            // already been here
            return;
    }

    if (routeThing.makeJumper) {
        jumperWillFit(next, routeThing);
    }

    // any way to skip viaWillFit or put it off until actually needed?
    if (crossLayer && !viaWillFit(next, routeThing.grid)) {
        return;
    }

    quint32 cost = routeThing.grid->at(gridPoint.x, gridPoint.y, gridPoint.z);
    if (cost == GridSource) {
        cost = 0;
    }
    if (crossLayer) {
        cost += ViaCost;
    }
    cost++;

    /*
    int increment = 5;
    // assume because of obstacles around the board that we can never be off grid from (next.x, next.y)
    switch(grid->at(next.x - 1, next.y, next.z)) {
        case GridObstacle:
        case GridSource:
        case GridTarget:
            increment--;
        default:
            break;
    }
    switch(grid->at(next.x + 1, next.y, next.z)) {
        case GridObstacle:
        case GridSource:
        case GridTarget:
            increment--;
        default:
            break;
    }
    switch(grid->at(next.x, next.y - 1, next.z)) {
        case GridObstacle:
        case GridSource:
        case GridTarget:
            increment--;
        default:
            break;
    }
    switch(grid->at(next.x, next.y + 1, next.z)) {
        case GridObstacle:
        case GridSource:
        case GridTarget:
            increment--;
        default:
            break;
    }
    next.cost += increment;
    */

    if (nextval != GridTarget) {
        routeThing.grid->setAt(next.x, next.y, next.z, cost);
    }

    next.cost = cost;
    //updateDisplay(next);
                
    next.cost += (m_costFunction)(QPoint(next.x, next.y), routeThing.gridTarget);
    routeThing.pq.push(next);
}

bool MazeRouter::viaWillFit(GridPoint & gridPoint, Grid * grid) {
    for (int y = -m_halfGridViaSize; y <= m_halfGridViaSize; y++) {
        for (int x = -m_halfGridViaSize; x <= m_halfGridViaSize; x++) {
            for (int z = 0; z < 1; z++) {
                switch(grid->at(gridPoint.x + x, gridPoint.y + y, z)) {
                    case GridObstacle:
                    case GridSource:
                    case GridTarget:
                        return false;
                    default:
                        break;
                }
            }
        }
    }
    return true;
}

void MazeRouter::jumperWillFit(GridPoint & gridPoint, RouteThing & routeThing) {
    double d = GraphicsUtils::distanceSqd(QPoint(gridPoint.x, gridPoint.y), routeThing.gridTarget);
    if (d >= routeThing.jumperDistance) return;

    for (int y = -m_halfGridJumperSize; y <= m_halfGridJumperSize; y++) {
        for (int x = -m_halfGridJumperSize; x <= m_halfGridJumperSize; x++) {
            // check each layer
            switch(routeThing.grid->at(gridPoint.x + x, gridPoint.y + y, 0)) {
                case GridObstacle:
                case GridSource:
                case GridTarget:
                    return;
                default:
                    break;
            }
        }
    }

    if (m_bothSidesNow) {
        for (int y = -m_halfGridJumperSize; y <= m_halfGridJumperSize; y++) {
            for (int x = -m_halfGridJumperSize; x <= m_halfGridJumperSize; x++) {
                switch(routeThing.grid->at(gridPoint.x + x, gridPoint.y + y, 1)) {
                    case GridObstacle:
                    case GridSource:
                    case GridTarget:
                        return;
                    default:
                        break;
                }
            }
        }
    }

    routeThing.jumperDistance = d;
    routeThing.jumperLocation = gridPoint;
}

void MazeRouter::updateDisplay(int iz) {
    QPixmap pixmap = QPixmap::fromImage(*m_displayImage[iz]);
    if (m_displayItem[iz] == NULL) {
        m_displayItem[iz] = new QGraphicsPixmapItem(pixmap);
        m_displayItem[iz]->setFlag(QGraphicsItem::ItemIsSelectable, false);
        m_displayItem[iz]->setFlag(QGraphicsItem::ItemIsMovable, false);
        //m_displayItem[iz]->setPos(iz == 1 ? m_maxRect.topLeft() : m_maxRect.topRight());
        m_displayItem[iz]->setPos(m_maxRect.topLeft());
        m_sketchWidget->scene()->addItem(m_displayItem[iz]);
        m_displayItem[iz]->setZValue(5000);
        //m_displayItem[iz]->setZValue(m_sketchWidget->viewLayers().value(iz == 0 ? ViewLayer::Copper0 : ViewLayer::Copper1)->nextZ());
        m_displayItem[iz]->setScale(m_maxRect.width() / m_displayImage[iz]->width());
        m_displayItem[iz]->setVisible(true);
    }
    else {
        m_displayItem[iz]->setPixmap(pixmap);
    }
    ProcessEventBlocker::processEvents();
}

void MazeRouter::updateDisplay(Grid * grid, int iz) {
    m_displayImage[iz]->fill(0);
    for (int y = 0; y < grid->y; y++) {
        for (int x = 0; x < grid->x; x++) {
            uint color;
            quint32 val = grid->at(x, y, iz);
            switch (val) {
                case GridObstacle:
                    color = 0xff000000;
                    break;
                case GridSource:
                    color = 0xff00ff00;
                    break;
                case GridTarget:
                    color = 0xffffff00;
                    break;
                case 0:
                    continue;
                default:
                    color = 0xffff00ff;
                    break;
            }

            m_displayImage[iz]->setPixel(x, y, color);
        }
    }

    updateDisplay(iz);
}

void MazeRouter::updateDisplay(GridPoint & gridPoint) {
    static int counter = 0;
    if (counter++ % 20 == 0) {
        m_displayImage[gridPoint.z]->setPixel(gridPoint.x, gridPoint.y, 0xffff0000);
        updateDisplay(gridPoint.z);
    }
}

void MazeRouter::clearExpansion(Grid * grid) {
    // TODO: keep a list of expansion points instead?

    for (int z = 0; z < grid->z; z++) {
        for (int y = 0; y < grid->y; y++) {
            for (int x = 0; x < grid->x; x++) {
                switch (grid->at(x, y, z)) {
                    case GridObstacle:
                    case 0:
                        break;
                    case GridTarget:
                    case GridSource:
                    default:
                        grid->setAt(x, y, z, 0);
                        break;
                }
            }
        }
    }
}

void MazeRouter::clearExpansionForJumper(Grid * grid, quint32 sourceOrTarget, std::priority_queue<GridPoint> & pq) {
    pq = std::priority_queue<GridPoint>();

    for (int z = 0; z < grid->z; z++) {
        for (int y = 0; y < grid->y; y++) {
            for (int x = 0; x < grid->x; x++) {
                switch (grid->at(x, y, z)) {
                    case GridObstacle:
                    case 0:
                        break;
                    case GridTarget:
                        if (sourceOrTarget == GridTarget) {
                            grid->setAt(x, y, z, GridSource);
                            GridPoint gp;
                            gp.x = x;
                            gp.y = y;
                            gp.z = z;
                            gp.cost = 0;
                            pq.push(gp);
                        }
                        break;
                    case GridSource:
                        if (sourceOrTarget == GridSource) {
                            GridPoint gp;
                            gp.x = x;
                            gp.y = y;
                            gp.z = z;
                            gp.cost = 0;
                            pq.push(gp);
                        }
                        break;
                    default:
                        grid->setAt(x, y, z, 0);
                        break;
                }
            }
        }
    }
}

void MazeRouter::initTraceDisplay() {
    m_displayImage[0]->fill(0);
    m_displayImage[1]->fill(0);
}

void MazeRouter::drawTrace(Trace & trace) {
    if (trace.gridPoints.count() == 0) {
        DebugDialog::debug("trace with no points");
        return;
    }

    int lastz = trace.gridPoints.at(0).z;
    foreach (GridPoint gridPoint, trace.gridPoints) {
        if (gridPoint.z != lastz) {
            for (int y = -m_halfGridViaSize; y <= m_halfGridViaSize; y++) {
                for (int x = -m_halfGridViaSize; x <= m_halfGridViaSize; x++) {
                    m_displayImage[1]->setPixel(x + gridPoint.x, y + gridPoint.y, 0x80ff0000);
                }
            }
        }
        else {
            uint color = (gridPoint.z == 0) ? 0xa0F28A00 : 0xa0FFCB33;
            m_displayImage[gridPoint.z]->setPixel(gridPoint.x, gridPoint.y, color);
        }
        lastz = gridPoint.z;
    }
    
    if (trace.flags) {
        GridPoint gridPoint = trace.gridPoints.first();
        for (int y = -m_halfGridJumperSize; y <= m_halfGridJumperSize; y++) {
            for (int x = -m_halfGridJumperSize; x <= m_halfGridJumperSize; x++) {
                m_displayImage[0]->setPixel(x + gridPoint.x, y + gridPoint.y, 0x800000ff);
            }
        }
    }
}

void MazeRouter::traceObstacles(QList<Trace> & traces, int netIndex, Grid * grid, int ikeepout) {
    // treat traces from previous nets as obstacles
    foreach (Trace trace, traces) {
        if (trace.netIndex == netIndex) continue;

        int lastZ = trace.gridPoints.at(0).z;
        foreach (GridPoint gridPoint, trace.gridPoints) {
            if (gridPoint.z != lastZ) {
                for (int y = -m_halfGridViaSize; y <= m_halfGridViaSize; y++) {
                    for (int x = -m_halfGridViaSize; x <= m_halfGridViaSize; x++) {
                        grid->setAt(gridPoint.x + x, gridPoint.y + y, 0, GridObstacle);
                        grid->setAt(gridPoint.x + x, gridPoint.y + y, 1, GridObstacle);
                    }
                }
            }
            else {
                for (int y = -ikeepout; y <= ikeepout; y++) {
                    for (int x = -ikeepout; x <= ikeepout; x++) {
                        grid->setAt(gridPoint.x + x, gridPoint.y + y, gridPoint.z, GridObstacle);
                    }
                }
            }
            lastZ = gridPoint.z;
        }

        if (trace.flags) {
            GridPoint gridPoint = trace.gridPoints.first();
            for (int y = -m_halfGridJumperSize; y <= m_halfGridJumperSize; y++) {
                for (int x = -m_halfGridJumperSize; x <= m_halfGridJumperSize; x++) {
                    grid->setAt(gridPoint.x + x, gridPoint.y + y, 0, GridObstacle);
                    if (m_bothSidesNow) {
                        grid->setAt(gridPoint.x + x, gridPoint.y + y, 1, GridObstacle);
                    }
                }
            }
        }
    }
}

void MazeRouter::cleanUpNets(NetList & netList) {
    foreach(Net * net, netList.nets) {
        delete net;
    }
    netList.nets.clear();
    Autorouter::cleanUpNets();
}

void MazeRouter::createTraces(NetList & netList, Score & bestScore, QUndoCommand * parentCommand) {
    QPointF topLeft = m_maxRect.topLeft();
    foreach (int netIndex, bestScore.ordering.order) {
        DebugDialog::debug(QString("tracing net %1").arg(netIndex));
        QList<Trace> traces = bestScore.traces.values(netIndex);
        qSort(traces.begin(), traces.end(), byOrder);
        QList<TraceWire *> newTraces;
        QList<Via *> newVias;
        QList<JumperItem *> newJumperItems;
    
        JumperItem * jumperItem = NULL;
        for (int tix = 0; tix < traces.count(); tix++) {
            Trace trace = traces.at(tix);
            QList<GridPoint> gridPoints = trace.gridPoints;
            // TODO: nicer curve-fitting
            removeColinear(gridPoints);
            removeSteps(gridPoints);

            if (trace.flags == JumperStart) {
                Trace trace2 = traces.at(tix + 1);
	            long newID = ItemBase::getNextID();
	            ViewGeometry viewGeometry;
	            ItemBase * itemBase = m_sketchWidget->addItem(m_sketchWidget->referenceModel()->retrieveModelPart(ModuleIDNames::JumperModuleIDName), 
												              ViewLayer::ThroughHoleThroughTop_TwoLayers, BaseCommand::CrossView, viewGeometry, newID, -1, NULL);

	            jumperItem = dynamic_cast<JumperItem *>(itemBase);
	            jumperItem->setAutoroutable(true);
	            m_sketchWidget->scene()->addItem(jumperItem);
                GridPoint gp = trace.gridPoints.first();
                QRectF gridRect1(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
                gp = trace2.gridPoints.first();
                QRectF gridRect2(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
	            jumperItem->resize(gridRect1.center(), gridRect2.center());
                addJumperToUndo(jumperItem, parentCommand);
                newJumperItems << jumperItem;
            }
            else if (trace.flags == JumperEnd) {
            }
            else jumperItem = NULL;

            Net * net = netList.nets.at(netIndex);
            bool onTraceS, onTraceD;
            QPointF traceAnchorS, traceAnchorD;
            ConnectorItem * sourceConnectorItem = NULL;
            if (jumperItem) {
                onTraceS = onTraceD = false;
                sourceConnectorItem = trace.flags == JumperStart ? jumperItem->connector0() : jumperItem->connector1();
            }
            else {
                sourceConnectorItem = findAnchor(gridPoints.first(), topLeft, net, newTraces, newVias, traceAnchorS, onTraceS);
            }
            if (sourceConnectorItem == NULL) continue;
            
            ConnectorItem * destConnectorItem = findAnchor(gridPoints.last(), topLeft, net, newTraces, newVias, traceAnchorD, onTraceD);
            if (destConnectorItem == NULL) continue;
            
            QPointF sourcePoint = sourceConnectorItem->sceneAdjustedTerminalPoint(NULL);
            QPointF destPoint = destConnectorItem->sceneAdjustedTerminalPoint(NULL);

            GridPoint gp = gridPoints.last();
            QRectF gridRect(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
            QPointF center = gridRect.center();
            if (!atLeast(center, destPoint)) {
                // don't need this last point
                gridPoints.takeLast();
            }

            gp = gridPoints.takeFirst();
            gridRect.setRect(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
            center = gridRect.center();
            if (!atLeast(center, sourcePoint)) {
                gp = gridPoints.takeFirst();
                gridRect.setRect(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
                center = gridRect.center();
            }

            int lastz = gp.z;
            ConnectorItem * nextSource = NULL;
            if (onTraceS) {
                if (!atLeast(sourcePoint, traceAnchorS)) {
                    onTraceS = false;
                }
                else if (atLeast(center, traceAnchorS)) {
                    onTraceS = false;
                }
            }
            if (onTraceS) {
                TraceWire * traceWire1 = drawOneTrace(sourcePoint, traceAnchorS, m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                addWireToUndo(traceWire1, parentCommand);
                addConnectionToUndo(sourceConnectorItem, traceWire1->connector0(), parentCommand);
                newTraces << traceWire1;
               
                TraceWire* traceWire2 = drawOneTrace(traceAnchorS, center, m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                addWireToUndo(traceWire2, parentCommand);
                addConnectionToUndo(traceWire1->connector1(), traceWire2->connector0(), parentCommand);
                nextSource = traceWire2->connector1();
                newTraces << traceWire2;
            }
            else {
                TraceWire * traceWire = drawOneTrace(sourcePoint, center, m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                addWireToUndo(traceWire, parentCommand);
                addConnectionToUndo(sourceConnectorItem, traceWire->connector0(), parentCommand);
                nextSource = traceWire->connector1();
                newTraces << traceWire;
            }

            foreach (GridPoint gp, gridPoints) {
                QRectF gridRect(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
                if (gp.z == lastz) {
                    TraceWire * traceWire = drawOneTrace(center, gridRect.center(), m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                    addWireToUndo(traceWire, parentCommand);
                    addConnectionToUndo(nextSource, traceWire->connector0(), parentCommand);
                    nextSource = traceWire->connector1();
                    newTraces << traceWire;
                }
                else {
	                long newID = ItemBase::getNextID();
	                ViewGeometry viewGeometry;
	                double ringThickness, holeSize;
	                m_sketchWidget->getViaSize(ringThickness, holeSize);
                    double halfVia = (ringThickness + ringThickness + holeSize) / 2;

	                viewGeometry.setLoc(QPointF(gridRect.center().x() - halfVia - Hole::OffsetPixels, gridRect.center().y() - halfVia - Hole::OffsetPixels));
	                ItemBase * itemBase = m_sketchWidget->addItem(m_sketchWidget->referenceModel()->retrieveModelPart(ModuleIDNames::ViaModuleIDName), 
										                ViewLayer::ThroughHoleThroughTop_TwoLayers, BaseCommand::SingleView, viewGeometry, newID, -1, NULL);

	                //DebugDialog::debug(QString("back from adding via %1").arg((long) itemBase, 0, 16));
	                Via * via = qobject_cast<Via *>(itemBase);
	                via->setAutoroutable(true);
	                via->setHoleSize(QString("%1in,%2in") .arg(holeSize / GraphicsUtils::SVGDPI) .arg(ringThickness / GraphicsUtils::SVGDPI), false);
                    addViaToUndo(via, parentCommand);
                    addConnectionToUndo(nextSource, via->connectorItem(), parentCommand);
                    nextSource = via->connectorItem();
                    newVias << via;
                    lastz = gp.z;
                }
                center = gridRect.center();
            }
            if (onTraceD) {
                if (!atLeast(destPoint, traceAnchorD)) {
                    onTraceD = false;
                }
                else if (!atLeast(center, traceAnchorD)) {
                    onTraceD = false;
                }
            }
            if (onTraceD) {
                TraceWire * traceWire1 = drawOneTrace(center, traceAnchorD, m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                addWireToUndo(traceWire1, parentCommand);
                addConnectionToUndo(nextSource, traceWire1->connector0(), parentCommand);
                newTraces << traceWire1;

                TraceWire * traceWire2 = drawOneTrace(traceAnchorD, destPoint, m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                addWireToUndo(traceWire2, parentCommand);
                addConnectionToUndo(traceWire1->connector1(), traceWire2->connector0(), parentCommand);
                addConnectionToUndo(traceWire2->connector1(), destConnectorItem, parentCommand);
                newTraces << traceWire2;
            }
            else {
                TraceWire * traceWire = drawOneTrace(center, destPoint, m_standardWireWidth, lastz == 0 ? ViewLayer::Bottom : ViewLayer::Top);
                addWireToUndo(traceWire, parentCommand);
                addConnectionToUndo(nextSource, traceWire->connector0(), parentCommand);
                addConnectionToUndo(traceWire->connector1(), destConnectorItem, parentCommand);
                newTraces << traceWire;
            }
        }

        foreach (TraceWire * traceWire, newTraces) {
            delete traceWire;
        }
        foreach (Via * via, newVias) {
            via->removeLayerKin();
            delete via;
        }
        foreach (JumperItem * jumperItem, newJumperItems) {
            jumperItem->removeLayerKin();
            delete jumperItem;
        }
    }
}

ConnectorItem * MazeRouter::findAnchor(GridPoint gp, QPointF topLeft, Net * net, QList<TraceWire *> & newTraces, QList<Via *> & newVias, QPointF & p, bool & onTrace) {
    QRectF gridRect(gp.x * m_standardWireWidth + topLeft.x(), gp.y * m_standardWireWidth + topLeft.y(), m_standardWireWidth, m_standardWireWidth);
    QList<TraceWire *> traceWires;
    foreach (QGraphicsItem * item, m_sketchWidget->scene()->items(gridRect)) {
        ConnectorItem * connectorItem = dynamic_cast<ConnectorItem *>(item);
        if (connectorItem) {
            if (!connectorItem->attachedTo()->isEverVisible()) continue;
            bool isCandidate = 
                (gp.z == 0 && m_sketchWidget->attachedToBottomLayer(connectorItem)) ||
                (gp.z == 1 && m_sketchWidget->attachedToTopLayer(connectorItem))
             ;
            if (!isCandidate) continue;

            if (net->net->contains(connectorItem)) ;
            else {
                TraceWire * traceWire = qobject_cast<TraceWire *>(connectorItem->attachedTo());
                if (traceWire == NULL) {
                    Via * via = qobject_cast<Via *>(connectorItem->attachedTo());
                    if (via == NULL) isCandidate = false;
                    else isCandidate = newVias.contains(via);
                }
                else isCandidate = newTraces.contains(traceWire);
            }
            if (!isCandidate) continue;

            onTrace = false;
            p = connectorItem->sceneAdjustedTerminalPoint(NULL);
            return connectorItem;
        }

        TraceWire * traceWire = dynamic_cast<TraceWire *>(item);
        if (traceWire == NULL) continue;
        if (!traceWire->isEverVisible()) continue;

        // only do traces if no connectorItem is found
        traceWires.append(traceWire);
    }

    foreach (TraceWire * traceWire, traceWires) {
        bool isCandidate = (gp.z == 0 && m_sketchWidget->attachedToBottomLayer(traceWire->connector0()))
                        || (gp.z == 1 && m_sketchWidget->attachedToTopLayer(traceWire->connector0()));
        if (!isCandidate) continue;

        if (newTraces.contains(traceWire)) ;
        else if (net->net->contains(traceWire->connector0())) ;
        else continue;

        onTrace = true;
        QPointF center = gridRect.center();
        QPointF p0 = traceWire->connector0()->sceneAdjustedTerminalPoint(NULL);
        QPointF p1 = traceWire->connector1()->sceneAdjustedTerminalPoint(NULL);
        double d0 = GraphicsUtils::distanceSqd(p0, center);
        double d1 = GraphicsUtils::distanceSqd(p1, center);
        double dx, dy, distanceSegment;
        bool atEndpoint;
        GraphicsUtils::distanceFromLine(center.x(), center.y(), p0.x(), p0.y(), p1.x(), p1.y(), dx, dy, distanceSegment, atEndpoint);
        if (atEndpoint) {
            DebugDialog::debug("at endpoint shouldn't happen");
        }
        p.setX(dx);
        p.setY(dy);
        if (d0 <= d1) {
            return traceWire->connector0();
        }
        else {
            return traceWire->connector1();
        }
    }

    DebugDialog::debug("overlap not found");
    return NULL;
}

void MazeRouter::removeColinear(QList<GridPoint> & gridPoints) {
    	// eliminate redundant colinear points
	int ix = 0;
	while (ix < gridPoints.count() - 2) {
		GridPoint p1 = gridPoints[ix];
		GridPoint p2 = gridPoints[ix + 1];
        if (p1.z == p2.z) {
		    GridPoint p3 = gridPoints[ix + 2];
            if (p2.z == p3.z) {
		        if (p1.x == p2.x && p2.x == p3.x) {
			        gridPoints.removeAt(ix + 1);
			        continue;
		        }
		        else if (p1.y == p2.y && p2.y == p3.y) {
			        gridPoints.removeAt(ix + 1);
			        continue;
		        }
            }
        }
		ix++;
	}
}

void MazeRouter::removeSteps(QList<GridPoint> & gridPoints) {
    // eliminate 45 degree runs
	for (int ix = 0; ix < gridPoints.count() - 2; ix++) {
        removeStep(ix, gridPoints);
	}
}

void MazeRouter::removeStep(int ix, QList<GridPoint> & gridPoints) {
	GridPoint p1 = gridPoints[ix];
	GridPoint p2 = gridPoints[ix + 1];
    if (p1.z != p2.z) return;

	GridPoint p3 = gridPoints[ix + 2];
    if (p2.z != p3.z) return;

    int dx1 = p2.x - p1.x;
    int dy1 = p2.y - p1.y;
    if ((qAbs(dx1) == 1 && dy1 == 0) || (dx1 == 0 && qAbs(dy1) == 1)) ;
    else return;

    int dx2 = p3.x - p2.x;
    int dy2 = p3.y - p2.y;
    bool step = false;
    if (dx1 == 0) {
        step = (dy2 == 0 && qAbs(dx2) == 1);
    }
    else {
        step = (dx2 == 0 && qAbs(dy2) == 1);
    }
    if (!step) return;

    int count = 1;
    int flag = 0;
    for (int jx = ix + 3; jx < gridPoints.count(); jx++, flag++) {
        GridPoint p4 = gridPoints[jx];
        int dx3 = p4.x - p3.x;
        int dy3 = p4.y - p3.y;
        if (flag % 2 == 0) {
            if (dx3 == dx1 && dy3 == dy1) {
                count++;
            }
            else break;
        }
        else {
            if (dx3 == dx2 && dy3 == dy2) {
                count++;
            }
            else break;
        }
        p2 = p3;
        p3 = p4;
    }
    while (--count >= 0) {
        gridPoints.removeAt(ix + 1);
    }
}

void MazeRouter::addConnectionToUndo(ConnectorItem * from, ConnectorItem * to, QUndoCommand * parentCommand) 
{
	ChangeConnectionCommand * ccc = new ChangeConnectionCommand(m_sketchWidget, BaseCommand::CrossView, 
											from->attachedToID(), from->connectorSharedID(),
											to->attachedToID(), to->connectorSharedID(),
											ViewLayer::specFromID(from->attachedToViewLayerID()),
											true, parentCommand);
	ccc->setUpdateConnections(false);
}

void MazeRouter::addViaToUndo(Via * via, QUndoCommand * parentCommand) {
	new AddItemCommand(m_sketchWidget, BaseCommand::CrossView, ModuleIDNames::ViaModuleIDName, via->viewLayerSpec(), via->getViewGeometry(), via->id(), false, -1, parentCommand);
	new SetPropCommand(m_sketchWidget, via->id(), "hole size", via->holeSize(), via->holeSize(), true, parentCommand);
	new CheckStickyCommand(m_sketchWidget, BaseCommand::SingleView, via->id(), false, CheckStickyCommand::RemoveOnly, parentCommand);
}

void MazeRouter::addJumperToUndo(JumperItem * jumperItem, QUndoCommand * parentCommand) {
	jumperItem->saveParams();
	QPointF pos, c0, c1;
	jumperItem->getParams(pos, c0, c1);

	new AddItemCommand(m_sketchWidget, BaseCommand::CrossView, ModuleIDNames::JumperModuleIDName, jumperItem->viewLayerSpec(), jumperItem->getViewGeometry(), jumperItem->id(), false, -1, parentCommand);
	new ResizeJumperItemCommand(m_sketchWidget, jumperItem->id(), pos, c0, c1, pos, c0, c1, parentCommand);
	new CheckStickyCommand(m_sketchWidget, BaseCommand::SingleView, jumperItem->id(), false, CheckStickyCommand::RemoveOnly, parentCommand);
}

void MazeRouter::insertTrace(Trace & newTrace, int netIndex, Score & currentScore, int viaCount) {
    if (newTrace.gridPoints.count() == 0) {
        DebugDialog::debug("trace with no points");
        return;
    }

    newTrace.netIndex = netIndex;
    newTrace.order = currentScore.traces.values(netIndex).count();
    currentScore.traces.insert(netIndex, newTrace);
    currentScore.routedCount.insert(netIndex, currentScore.routedCount.value(netIndex) + 1);
    currentScore.totalRoutedCount++;
    currentScore.viaCount.insert(netIndex, currentScore.viaCount.value(netIndex, 0) + viaCount);
    currentScore.totalViaCount += viaCount;
    drawTrace(newTrace);
}

