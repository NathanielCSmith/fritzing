/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2010 Fachhochschule Potsdam - http://fh-potsdam.de

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

#include "miniview.h"
#include "../debugdialog.h"
#include "../fgraphicsscene.h"
#include "miniviewcontainer.h"

static const QColor NormalColor(0x70, 0x70, 0x70);
static const QColor HoverColor(0x00, 0x00, 0x00);
static const QColor PressedColor(84, 24, 44 /*0xff, 0xff, 0xff */);
static const int FontPixelSize = 11;
static const QString FontFamily = "Droid Sans";

MiniView::MiniView(QWidget *parent )
	: QFrame(parent)
{
	m_graphicsView = NULL;
	m_selected = false;
	m_updateSceneTimer.setSingleShot(true);
	m_updateSceneTimer.setInterval(250);
	connect(&m_updateSceneTimer, SIGNAL(timeout()), this, SLOT(reallyUpdateScene()));
}

MiniView::~MiniView()
{
}

void MiniView::setTitle(const QString & title) {
	m_title = title;
	QFont font;
	font.setFamily(FontFamily);
	font.setWeight(QFont::Bold);
	font.setPixelSize(FontPixelSize);
	setFont(font);
	QFontMetrics metrics = fontMetrics();
	QRect br = metrics.boundingRect(m_title);
	parentWidget()->setMinimumWidth(br.width());
}

void MiniView::paintEvent(QPaintEvent * event) {
	QFrame::paintEvent(event);

	QPainter painter(this);
	QRect vp = painter.viewport(); 
	if (m_graphicsView && m_graphicsView->scene()) {
		painter.fillRect(0, 0, vp.width(), vp.height(), m_graphicsView->scene()->backgroundBrush());
		QRectF sr = m_graphicsView->scene()->sceneRect();
		int cw = width();
		int ch = qRound(sr.height() * cw / sr.width());
		if (ch > height()) {
			ch = height();
			cw = qRound(sr.width() * ch / sr.height());
		}
		m_sceneRect.setCoords((width() - cw) / 2, (height() - ch) / 2, cw, ch);
		m_graphicsView->scene()->render(&painter, m_sceneRect, sr, Qt::KeepAspectRatio);
	}

	QPen pen(m_titleColor, 1);
	painter.setPen(pen);
	QFont font;
	font.setFamily(FontFamily);
	font.setWeight(m_titleWeight);
	font.setPixelSize(FontPixelSize);
	painter.setFont(font);
	QFontMetrics metrics = painter.fontMetrics();
	m_lastHeight = metrics.descent() + metrics.ascent();
	int h = 0;  // metrics.descent();
	int y = vp.bottom() - h - 2;
	QRect br = metrics.boundingRect(m_title);
	int x = vp.left() + ((vp.width() - br.width()) / 2);
	painter.drawText(QPointF(x, y), m_title);

}


void MiniView::setView(QGraphicsView * view) {
	if (m_graphicsView) {
		if (m_graphicsView->scene()) {
			disconnect(m_graphicsView->scene(), SIGNAL(void sceneRectChanged(QRectF)), this, SLOT(void updateSceneRect()));
			disconnect(m_graphicsView->scene(), SIGNAL(changed(QList<QRectF>)), this, SLOT(updateScene()));

		}
	}
	m_graphicsView = view;
	if (view->scene()) {
		connect(view->scene(), SIGNAL(sceneRectChanged(QRectF)), this, SLOT(updateSceneRect()));
		connect(view->scene(), SIGNAL(changed(QList<QRectF>)), this, SLOT(updateScene()));
	}
}

void MiniView::updateScene() 
{
	m_updateSceneTimer.stop();
	m_updateSceneTimer.start();
}

void MiniView::updateSceneRect() 
{
	updateScene();
	emit rectChangedSignal();
}

void MiniView::resizeEvent ( QResizeEvent * event )
{
	Q_UNUSED(event);
	updateScene();
	emit rectChangedSignal();
}

void MiniView::mousePressEvent(QMouseEvent *) {
	// for some reason, this mousePressEvent isn't detected by setting an event filter on the miniview
	// maybe because the event happens in the scene and get swallowed before the filter gets it?
	emit miniViewMousePressedSignal();
}

QGraphicsView * MiniView::view() {
	return m_graphicsView;
}


void MiniView::navigatorMousePressedSlot(MiniViewContainer * miniViewContainer) {
	if (miniViewContainer == this->parentWidget()) {
		m_selected = true;
		m_titleColor = PressedColor;
		m_titleWeight = QFont::Bold;
		qobject_cast<MiniViewContainer *>(parentWidget())->hideHandle(false);
	}
	else {
		m_selected = false;
		m_titleWeight = QFont::Normal;
		m_titleColor = NormalColor;
		qobject_cast<MiniViewContainer *>(parentWidget())->hideHandle(true);
	}

	this->update();			

}

void MiniView::navigatorMouseEnterSlot(MiniViewContainer * miniViewContainer) {
	if (miniViewContainer == this->parentWidget()) {
		if (!m_selected) {
			m_titleColor = HoverColor;
			this->update();	
		}
	}
}

void MiniView::navigatorMouseLeaveSlot(MiniViewContainer * miniViewContainer) {
	if (miniViewContainer == this->parentWidget()) {
		if (!m_selected) {
			m_titleColor = NormalColor;
			this->update();	
		}
	}
}

void MiniView::reallyUpdateScene() 
{
	this->update();
}

QRectF MiniView::sceneRect() {
	return m_sceneRect;
}
