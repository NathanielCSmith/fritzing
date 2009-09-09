/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2009 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 2676 $:
$Author: cohen@irascible.com $:
$Date: 2009-03-21 03:10:39 +0100 (Sat, 21 Mar 2009) $

********************************************************************/


#ifndef PARTSEDITORVIEW_H_
#define PARTSEDITORVIEW_H_

#include "../sketchwidget.h"
#include "../connectorshared.h"
#include "partseditorpaletteitem.h"
#include "partseditorconnectorspaletteitem.h"
#include "partseditorconnectorsconnectoritem.h"


class PartsEditorView : public SketchWidget {
	Q_OBJECT

	public:
		PartsEditorView(
			ViewIdentifierClass::ViewIdentifier, QDir tempDir,
			bool showingTerminalPoints, QGraphicsItem *startItem=0,
			QWidget *parent=0, int size=150, bool deleteModelPartOnClearScene=false);
		~PartsEditorView();

		// general
		QDir tempFolder();
		bool isEmpty();
		ViewLayer::ViewLayerID connectorLayerId();
		QString terminalIdForConnector(const QString &connId);
		void addFixedToBottomRight(QWidget *widget);
		bool imageLoaded();

		// specs
		void loadSvgFile(ModelPart * modelPart);
		void copySvgFileToDestiny(const QString &partFileName);

		const QString svgFilePath();
		const SvgAndPartFilePath& svgFileSplit();

		// conns
		void drawConector(Connector *conn, bool showTerminalPoint);
		void removeConnector(const QString &connId);
		void inFileDefinedConnectorChanged(PartsEditorConnectorsConnectorItem *connItem);
		void aboutToSave();

		void showTerminalPoints(bool show);
		bool showingTerminalPoints();

		QString svgIdForConnector(const QString &connId);
		PartsEditorConnectorsPaletteItem *myItem();

		bool connsPosOrSizeChanged();

	public slots:
		// general
		void loadFromModel(PaletteModel *paletteModel, ModelPart * modelPart);
		void addItemInPartsEditor(ModelPart * modelPart, SvgAndPartFilePath * svgFilePath);

		// specs
		void loadFile();
		void loadSvgFile(const QString& origPath);
		void updateModelPart(const QString& origPath);

		// conns
		void informConnectorSelection(const QString& connId);
		void informConnectorSelectionFromView(const QString& connId);
		void setMismatching(ViewIdentifierClass::ViewIdentifier viewId, const QString &id, bool mismatching);

	protected slots:
		void recoverTerminalPointsState();

	signals:
		// conns
		void connectorsFound(ViewIdentifierClass::ViewIdentifier viewId, const QList<Connector*> &conns);
		void svgFileLoadNeeded(const QString &filepath);
		void connectorSelected(const QString& connId);
		void removeTerminalPoint(const QString &connId, ViewIdentifierClass::ViewIdentifier vid);

	protected:
		// general
		PartsEditorPaletteItem *newPartsEditorPaletteItem(ModelPart * modelPart);
		PartsEditorPaletteItem *newPartsEditorPaletteItem(ModelPart * modelPart, SvgAndPartFilePath *path);

		void setDefaultBackground();
		void clearScene();
		void fitCenterAndDeselect();
		void removeConnectors();
		void addDefaultLayers();

		void wheelEvent(QWheelEvent* event);
		void drawBackground(QPainter *painter, const QRectF &rect);

		ItemBase * addItemAux(ModelPart * modelPart, const ViewGeometry & viewGeometry, long id, long originalModelIndex, AddDeleteItemCommand * originatingCommand, PaletteItem* paletteItem, bool doConnectors);

		ModelPart *createFakeModelPart(SvgAndPartFilePath *svgpath);
		ModelPart *createFakeModelPart(const QHash<QString,StringPair*> &connIds, const QStringList &layers, const QString &svgFilePath);

		const QHash<QString,StringPair*> getConnectorIds(const QString &path);
		void getConnectorIdsAux(QHash<QString,StringPair*> &retval, QDomElement &docElem);
		const QStringList getLayers(const QString &path);

		QString getOrCreateViewFolderInTemp();
		bool ensureFilePath(const QString &filePath);

		QString findConnectorLayerId(QDomDocument *svgDom);
		bool findConnectorLayerIdAux(QString &result, QDomElement &docElem, QStringList &prevLayers);
		bool terminalIdForConnectorIdAux(QString &result, const QString &connId, QDomElement &docElem);
		QString getLayerFileName(ModelPart * modelPart);


		// SVG fixing
		void beforeSVGLoading(const QString &filename, bool &canceled);
		bool isIllustratorFile(const QString &fileContent);
		bool fixPixelDimensionsIn(QString &fileContent, const QString &filename);
		bool fixViewboxOrigin(QString &fileContent, const QString &filename);
		bool fixFonts(QString &fileContent, const QString &filename, bool &canceled);
		bool cleanXml(QString &bytes, const QString & filename);
		bool removeFontFamilySingleQuotes(QString &fileContent, const QString &filename);
		bool fixUnavailableFontFamilies(QString &fileContent, const QString &filename, bool &canceled);
		bool pxToInches(QDomElement &elem, const QString &attrName, const QString &filename);
		bool moveViewboxToTopLeftCorner(QDomElement &elem, const QString &filename);
		QSet<QString> getAttrFontFamilies(const QString &fileContent);
		QSet<QString> getFontFamiliesInsideStyleTag(const QString &fileContent);
		QString removeXMLEntities(QString svgContent);


		// specs
		void setSvgFilePath(const QString &filePath);
		void copyToTempAndRenameIfNecessary(SvgAndPartFilePath *filePathOrig);
		QString createSvgFromImage(const QString &filePath);

		QString setFriendlierSvgFileName(const QString &partFileName);


		// conns
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);
		void connectItem();
		void createConnector(Connector *conn, const QSize &connSize, bool showTerminalPoint);
		void setItemProperties();
		bool isSupposedToBeRemoved(const QString& id);

		bool addConnectorsIfNeeded(QDomDocument *svgDom, const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId);
		bool removeConnectorsIfNeeded(QDomElement &docEle);
		bool updateTerminalPoints(QDomDocument *svgDom, const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId);
		QString svgIdForConnector(Connector* conn, const QString &connId);

		void updateSvgIdLayer(const QString &connId, const QString &terminalId, const QString &connectorsLayerId);
		void removeTerminalPoints(const QStringList &tpIdsToRemove, QDomElement &docElem);
		void addNewTerminalPoints(
				const QList<PartsEditorConnectorsConnectorItem*> &connsWithNewTPs, QDomDocument *svgDom,
				const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId
		);
		QRectF mapFromSceneToSvg(const QRectF &sceneRect, const QSizeF &defaultSize, const QRectF &viewBox);
		void addRectToSvg(QDomDocument* svgDom, const QString &id, const QRectF &rect, const QString &connectorsLayerId);
		bool addRectToSvgAux(QDomElement &docElem, const QString &connectorsLayerId, QDomElement &rectElem);



		PartsEditorPaletteItem *m_item; // just one item per view
		QDir m_tempFolder;
		bool m_deleteModelPartOnSceneClear;
		QGraphicsItem *m_startItem;

		SvgAndPartFilePath *m_svgFilePath;
		QString m_originalSvgFilePath;

		QHash<QString /*id*/,PartsEditorConnectorsConnectorItem*> m_drawnConns;
		QStringList m_removedConnIds;

		QString m_lastSelectedConnId;
		bool m_showingTerminalPoints;
		bool m_showingTerminalPointsBackup;
		QTimer *m_terminalPointsTimer;

		QList<QWidget* > m_fixedWidgets;

	protected:
		static int ConnDefaultWidth;
		static int ConnDefaultHeight;
};

#endif /* PARTSEDITORVIEW_H_ */
