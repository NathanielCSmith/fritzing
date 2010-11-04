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

#include "modelpartshared.h"
#include "../connectors/connectorshared.h"
#include "../debugdialog.h"
#include "../connectors/busshared.h"

#include <QHash>

ModelPartShared::ModelPartShared() {
	commonInit();

	m_domDocument = NULL;
	m_path = "";
}

ModelPartShared::ModelPartShared(QDomDocument * domDocument, const QString & path) {
	commonInit();

	m_domDocument = domDocument;
	m_path = path;

	if (domDocument) {
		QDomElement root = domDocument->documentElement();
		if (root.isNull()) {
			return;
		}

		if (root.tagName() != "module") {
			return;
		}

		loadTagText(root, "title", m_title);
		loadTagText(root, "label", m_label);
		loadTagText(root, "version", m_version);
		loadTagText(root, "author", m_author);
		loadTagText(root, "description", m_description);
		loadTagText(root, "taxonomy", m_taxonomy);
		loadTagText(root, "date", m_date);
		QDomElement version = root.firstChildElement("version");
		if (!version.isNull()) {
			m_replacedby = version.attribute("replacedby");
		}

		populateTagCollection(root, m_tags, "tags");
		populateTagCollection(root, m_properties, "properties", "name");

		m_moduleID = root.attribute("moduleId", "");

		QDomElement views = root.firstChildElement("views");
		if (!views.isNull()) {
			QDomElement view = views.firstChildElement();
			while (!view.isNull()) {
				ViewIdentifierClass::ViewIdentifier viewIdentifier = ViewIdentifierClass::idFromXmlName(view.nodeName());
				QDomElement layers = view.firstChildElement("layers");
				if (!layers.isNull()) {
					setHasBaseNameFor(viewIdentifier, layers.attribute("image"));
					QDomElement layer = layers.firstChildElement("layer");
					while (!layer.isNull()) {
						ViewLayer::ViewLayerID viewLayerID = ViewLayer::viewLayerIDFromXmlString(layer.attribute("layerId"));
						if (viewLayerID != ViewLayer::UnknownLayer) {
							setHasViewFor(viewIdentifier, viewLayerID);
						}
						layer = layer.nextSiblingElement("layer");
					}
				}

				view = view.nextSiblingElement();
			}
		}

	}
}

void ModelPartShared::commonInit() {
	m_flippedSMD = false;
	m_moduleID = "";
	m_connectorsInitialized = false;
	m_ignoreTerminalPoints = false;
	m_partlyLoaded = false;
	m_needsCopper1 = false;
}

ModelPartShared::~ModelPartShared() {
	foreach (ConnectorShared * connectorShared, m_connectorSharedHash.values()) {
		delete connectorShared;
	}
	m_connectorSharedHash.clear();

	foreach (ConnectorShared * connectorShared, m_deletedList) {
		delete connectorShared;
	}
	m_deletedList.clear();

	foreach (BusShared * busShared, m_buses.values()) {
		delete busShared;
	}
	m_buses.clear();

	if (m_domDocument) {
		delete m_domDocument;
		m_domDocument = NULL;
	}
}

void ModelPartShared::loadTagText(QDomElement parent, QString tagName, QString &field) {
	QDomElement tagElement = parent.firstChildElement(tagName);
	if (!tagElement.isNull()) {
		field = tagElement.text();
	}
}

void ModelPartShared::populateTagCollection(QDomElement parent, QStringList &list, const QString &tagName) {
	QDomElement bag = parent.firstChildElement(tagName);
	if (!bag.isNull()) {
		QDomNodeList childs = bag.childNodes();
		for(int i = 0; i < childs.size(); i++) {
			QDomNode child = childs.item(i);
			if(!child.isComment()) {
				list << child.toElement().text();
			}
		}
	}
}

void ModelPartShared::populateTagCollection(QDomElement parent, QHash<QString,QString> &hash, const QString &tagName, const QString &attrName) {
	QDomElement bag = parent.firstChildElement(tagName);
	if (!bag.isNull()) {
		QDomNodeList childs = bag.childNodes();
		for(int i = 0; i < childs.size(); i++) {
			QDomNode child = childs.item(i);
			if(!child.isComment()) {
				QString name = child.toElement().attribute(attrName);
				QString value = child.toElement().text();
				hash.insert(name.toLower().trimmed(),value);
			}
		}
	}
}

void ModelPartShared::setDomDocument(QDomDocument * domDocument) {
	if (m_domDocument) {
		delete m_domDocument;
	}
	m_domDocument = domDocument;
}

QDomDocument* ModelPartShared::domDocument() {
	if (m_partlyLoaded) {
		loadDocument();
	}

	return m_domDocument;
}

const QString & ModelPartShared::title() {
	return m_title;
}

void ModelPartShared::setTitle(QString title) {
	m_title = title;
}

const QString & ModelPartShared::label() {
	return m_label;
}

void ModelPartShared::setLabel(QString label) {
	m_label = label;
}

const QString & ModelPartShared::uri() {
	return m_uri;
}

void ModelPartShared::setUri(QString uri) {
	m_uri = uri;
}

const QString & ModelPartShared::version() {
	return m_version;
}

void ModelPartShared::setVersion(QString version) {
	m_version = version;
}

const QString & ModelPartShared::author() {
	return m_author;
}

void ModelPartShared::setAuthor(QString author) {
	m_author = author;
}

const QString & ModelPartShared::description() {
	return m_description;
}

void ModelPartShared::setDescription(QString description) {
	m_description = description;
}

const QDate & ModelPartShared::date() {
	// 	return *new QDate(QDate::fromString(m_date,Qt::ISODate));   // causes memory leak
	static QDate tempDate;
	tempDate = QDate::fromString(m_date,Qt::ISODate);
	return tempDate;
}

void ModelPartShared::setDate(QDate date) {
	m_date = date.toString(Qt::ISODate);
}

const QString & ModelPartShared::dateAsStr() {
	return m_date;
}

void ModelPartShared::setDate(QString date) {
	m_date = date;
}

const QStringList & ModelPartShared::tags() {
	return m_tags;
}
void ModelPartShared::setTags(const QStringList &tags) {
	m_tags = tags;
}

QString ModelPartShared::family() {
	return m_properties.value("family");
}
void ModelPartShared::setFamily(const QString &family) {
	m_properties.insert("family",family);
}

QHash<QString,QString> & ModelPartShared::properties() {
	return m_properties;
}
void ModelPartShared::setProperties(const QHash<QString,QString> &properties) {
	m_properties = properties;
}

const QString & ModelPartShared::path() {
	return m_path;
}
void ModelPartShared::setPath(QString path) {
	m_path = path;
}

const QString & ModelPartShared::taxonomy() {
	return m_taxonomy;
}

void ModelPartShared::setTaxonomy(QString taxonomy) {
	m_taxonomy = taxonomy;
}

const QString & ModelPartShared::moduleID() {
	return m_moduleID;
}
void ModelPartShared::setModuleID(QString moduleID) {
	m_moduleID = moduleID;
}

const QList< QPointer<ConnectorShared> > ModelPartShared::connectorsShared() {
	return m_connectorSharedHash.values();
}

void ModelPartShared::setConnectorsShared(QList< QPointer<ConnectorShared> > connectors) {
	for (int i = 0; i < connectors.size(); i++) {
		ConnectorShared* cs = connectors[i];
		m_connectorSharedHash[cs->id()] = cs;
	}
}

void ModelPartShared::resetConnectorsInitialization() {
	m_connectorsInitialized = false;

	foreach (ConnectorShared * cs, m_connectorSharedHash.values()) {
		// due to craziness in the parts editor
		m_deletedList.append(cs);
	}
	m_connectorSharedHash.clear();
}

void ModelPartShared::initConnectors() {
	if (m_connectorsInitialized)
		return;

	if (m_partlyLoaded) {
		loadDocument();
	}

	if (m_domDocument == NULL) {
		return;
	}

	m_connectorsInitialized = true;
	QDomElement root = m_domDocument->documentElement();
	if (root.isNull()) {
		return;
	}

	QDomElement connectors = root.firstChildElement("connectors");
	if (connectors.isNull())
		return;

	m_ignoreTerminalPoints = (connectors.attribute("ignoreTerminalPoints").compare("true", Qt::CaseInsensitive) == 0);

	//DebugDialog::debug(QString("part:%1 %2").arg(m_moduleID).arg(m_title));
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		ConnectorShared * connectorShared = new ConnectorShared(connector);
		m_connectorSharedHash.insert(connectorShared->id(), connectorShared);

		connector = connector.nextSiblingElement("connector");
	}

	QDomElement buses = root.firstChildElement("buses");
	if (!buses.isNull()) {
		QDomElement busElement = buses.firstChildElement("bus");
		while (!busElement.isNull()) {
			BusShared * busShared = new BusShared(busElement, m_connectorSharedHash);
			m_buses.insert(busShared->id(), busShared);

			busElement = busElement.nextSiblingElement("bus");
		}
	}

	//DebugDialog::debug(QString("model %1 has %2 connectors and %3 bus connectors").arg(this->title()).arg(m_connectorSharedHash.count()).arg(m_buses.count()) );


}

const QHash<QString, BusShared *> & ModelPartShared::buses() {
	return m_buses;
}

ConnectorShared * ModelPartShared::getConnectorShared(const QString & id) {
	return m_connectorSharedHash.value(id);
}

BusShared * ModelPartShared::bus(const QString & busID) {
	return m_buses.value(busID);
}

bool ModelPartShared::ignoreTerminalPoints() {
	return m_ignoreTerminalPoints;
}

void ModelPartShared::copy(ModelPartShared* other) {
	setAuthor(other->author());
	setConnectorsShared(other->connectorsShared());
	setDate(other->date());
	setLabel(other->label());
	setDescription(other->description());
	setFamily(other->family());
	setProperties(other->properties());
	setTags(other->tags());
	setTaxonomy(other->taxonomy());
	setTitle(other->title());
	setUri(other->uri());
	setVersion(other->version());
	foreach (ViewIdentifierClass::ViewIdentifier viewIdentifier, other->m_hasViewFor.keys()) {
		foreach (ViewLayer::ViewLayerID viewLayerID, other->m_hasViewFor.values(viewIdentifier)) {
			setHasViewFor(viewIdentifier, viewLayerID);
		}
	}
	foreach (ViewIdentifierClass::ViewIdentifier viewIdentifier, other->m_hasBaseNameFor.keys()) {
		setHasBaseNameFor(viewIdentifier, other->hasBaseNameFor(viewIdentifier));
	}
}

void ModelPartShared::setProperty(const QString & key, const QString & value) {
	m_properties.insert(key, value);
}

const QString & ModelPartShared::replacedby() {
	return m_replacedby;
}

void ModelPartShared::setReplacedby(const QString & replacedby) {
	m_replacedby = replacedby;
}

void ModelPartShared::setFlippedSMD(bool f) {
	m_flippedSMD = f;
}

bool ModelPartShared::flippedSMD() {
	return m_flippedSMD;
}

bool ModelPartShared::needsCopper1() {
	return m_needsCopper1;
}

void ModelPartShared::connectorIDs(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID, QStringList & connectorIDs, QStringList & terminalIDs) {
	foreach (ConnectorShared * connector, m_connectorSharedHash.values()) {
		connectorIDs.append(connector->pin(viewIdentifier, viewLayerID));
		terminalIDs.append(connector->terminal(viewIdentifier, viewLayerID));
	}
}

void ModelPartShared::setPartlyLoaded(bool partlyLoaded) {
	m_partlyLoaded = partlyLoaded;
}

void ModelPartShared::loadDocument() {
	m_partlyLoaded = false;

	//DebugDialog::debug("loading document " + m_moduleID);

	QFile file(m_path);
	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument * doc = new QDomDocument();

	if (!doc->setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		delete doc;
	}
	else {
		m_domDocument = doc;
		flipSMDAnd();
	}
}

void ModelPartShared::flipSMDAnd() {
	QDomElement root = m_domDocument->documentElement();
	QDomElement views = root.firstChildElement("views");
	if (views.isNull()) return;

	QDomElement pcb = views.firstChildElement("pcbView");
	if (pcb.isNull()) return;

	QDomElement layers = pcb.firstChildElement("layers");
	if (layers.isNull()) return;

	QString c1String = ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1);
	QString c0String = ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper0);
	QString s0String = ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen0);
	QString s1String = ViewLayer::viewLayerXmlNameFromID(ViewLayer::Silkscreen1);
	
	QDomElement c0;
	QDomElement c1;
	QDomElement s0;
	QDomElement s1;

	QDomElement layer = layers.firstChildElement("layer");
	while (!layer.isNull()) {
		QString layerID = layer.attribute("layerId");
		if (layerID.compare(c1String) == 0) {
			c1 = layer;
		}
		else if (layerID.compare(c0String) == 0) {
			c0 = layer;
		}
		else if (layerID.compare(s0String) == 0) {
			s0 = layer;
		}
		else if (layerID.compare(s1String) == 0) {
			s1 = layer;
		}

		layer = layer.nextSiblingElement("layer");
	}

	if (!c0.isNull()) {
		if (checkNeedsCopper1(c0, c1)) {
			setHasViewFor(ViewIdentifierClass::PCBView, ViewLayer::Copper1);
		}
		return;
	}
	if (c1.isNull()) return;

	setFlippedSMD(true);

	if (c0.isNull()) {
		c0 = m_domDocument->createElement("layer");
		c0.setAttribute("layerId", c0String);
		layers.appendChild(c0);
		setHasViewFor(ViewIdentifierClass::PCBView, ViewLayer::Copper0);
	}

	c0.setAttribute("flipSMD", "true");

	if (!s1.isNull() && s0.isNull()) {
		s0 = m_domDocument->createElement("layer");
		s0.setAttribute("layerId", s0String);
		layers.appendChild(s0);
		setHasViewFor(ViewIdentifierClass::PCBView, ViewLayer::Silkscreen0);
	}

	if (!s0.isNull()) {
		s0.setAttribute("flipSMD", "true");
	}

	QDomElement connectors = root.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		views = connector.firstChildElement("views");
		pcb = views.firstChildElement("pcbView");
		QDomElement p = pcb.firstChildElement("p");
		QList<QDomElement> newPs;
		while (!p.isNull()) {
			QString l = p.attribute("layer");
			if (l == c1String) {
				QDomElement newP = m_domDocument->createElement("p");
				newPs.append(newP);
				newP.setAttribute("layer", c0String);
				newP.setAttribute("flipSMD", "true");
				newP.setAttribute("svgId", p.attribute("svgId"));
				QString terminalId = p.attribute("terminalId");
				if (!terminalId.isEmpty()) {
					newP.setAttribute("terminalId", terminalId);
				}
			}
			p = p.nextSiblingElement("p");
		}
		foreach(QDomElement p, newPs) {
			pcb.appendChild(p);
		}

		connector = connector.nextSiblingElement("connector");
	}

#ifndef QT_NO_DEBUG
	QString temp = m_domDocument->toString();
	Q_UNUSED(temp);
	//DebugDialog::debug(temp);
#endif
}

bool ModelPartShared::hasViewFor(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	return m_hasViewFor.values(viewIdentifier).count() > 0;
}

bool ModelPartShared::hasViewFor(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID) {
	return m_hasViewFor.values(viewIdentifier).contains(viewLayerID);
}

void ModelPartShared::setHasViewFor(ViewIdentifierClass::ViewIdentifier viewIdentifier, ViewLayer::ViewLayerID viewLayerID) {
	m_hasViewFor.insert(viewIdentifier, viewLayerID);
}

QString ModelPartShared::hasBaseNameFor(ViewIdentifierClass::ViewIdentifier viewIdentifier) {
	return m_hasBaseNameFor.value(viewIdentifier, "");
}

void ModelPartShared::setHasBaseNameFor(ViewIdentifierClass::ViewIdentifier viewIdentifier, const QString & value) {
	m_hasBaseNameFor.insert(viewIdentifier, value);
}

bool ModelPartShared::checkNeedsCopper1(QDomElement & copper0, QDomElement & copper1) 
{
	if (!m_replacedby.isEmpty()) return false;

	bool ok;
	qreal versionNumber = m_version.toDouble(&ok);
	if (ok) {
		if (versionNumber >= 4.0) return false;
	}

	QString c1String = ViewLayer::viewLayerXmlNameFromID(ViewLayer::Copper1);

	if (copper1.isNull()) {					
		QDomElement c1 = m_domDocument->createElement("layer");
		c1.setAttribute("layerId", c1String);
		copper0.parentNode().appendChild(c1);
		m_needsCopper1 = true;
	}

	QDomElement root = m_domDocument->documentElement();
	QDomElement connectors = root.firstChildElement("connectors");
	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		QDomElement views = connector.firstChildElement("views");
		QDomElement pcb = views.firstChildElement("pcbView");
		QDomElement p = pcb.firstChildElement("p");
		bool hasCopper1 = false;
		while (!p.isNull()) {
			QString l = p.attribute("layer");
			if (l == c1String) {
				hasCopper1 = true;
				break;
			}
			p = p.nextSiblingElement("p");
		}
		if (!hasCopper1) {
			p = pcb.firstChildElement("p");
			QDomElement newP = m_domDocument->createElement("p");
			newP.setAttribute("layer", c1String);
			newP.setAttribute("svgId", p.attribute("svgId"));
			QString terminalId = p.attribute("terminalId");
			if (!terminalId.isEmpty()) {
				newP.setAttribute("svgId", terminalId);
			}
			pcb.appendChild(newP);
			m_needsCopper1 = true;
		}

		connector = connector.nextSiblingElement("connector");
	}

	return m_needsCopper1;

	//QString test = m_domDocument->toString();
	//DebugDialog::debug("test " + test);
}
