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

$Revision$:
$Author$:
$Date$

********************************************************************/

#include "palettemodel.h"
#include <QFile>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QDomElement>

#include "../debugdialog.h"
#include "modelpart.h"
#include "../version/version.h"
#include "../layerattributes.h"
#include "../utils/folderutils.h"
#include "../items/moduleidnames.h"

#ifndef QT_NO_DEBUG
bool PaletteModel::CreateAllPartsBinFile = true;
#else
bool PaletteModel::CreateAllPartsBinFile = false;
#endif
bool PaletteModel::CreateNonCorePartsBinFile = true;

bool JustAppendAllPartsInstances = false;

QString PaletteModel::AllPartsBinFilePath = ___emptyString___;
QString PaletteModel::NonCorePartsBinFilePath = ___emptyString___;


const QString InstanceTemplate(
        		"\t\t<instance moduleIdRef=\"%1\" path=\"%2\">\n"
				"\t\t\t<views>\n"
        		"\t\t\t\t<iconView layer=\"icon\">\n"
        		"\t\t\t\t\t<geometry z=\"-1\" x=\"-1\" y=\"-1\"></geometry>\n"
        		"\t\t\t\t</iconView>\n"
        		"\t\t\t</views>\n"
        		"\t\t</instance>\n");

PaletteModel::PaletteModel() : ModelBase(true) {
	m_loadedFromFile = false;
	m_loadingCore = false;
	m_loadingContrib = false;
}

PaletteModel::PaletteModel(bool makeRoot, bool doInit) : ModelBase( makeRoot ) {
	m_loadedFromFile = false;
	m_loadingCore = false;
	m_loadingContrib = false;

	if (doInit)
		init();
}

void PaletteModel::init() {
	loadParts();
	if (m_root == NULL) {
	    QMessageBox::information(NULL, QObject::tr("Fritzing"),
	                             QObject::tr("No parts found.") );
	}
}

void PaletteModel::initNames() {
	AllPartsBinFilePath = FolderUtils::getApplicationSubFolderPath("bins")+"/allParts.dbg" + FritzingBinExtension;
	NonCorePartsBinFilePath = FolderUtils::getApplicationSubFolderPath("bins")+"/nonCoreParts" + FritzingBinExtension;
}

ModelPart * PaletteModel::retrieveModelPart(const QString & moduleID) {
	ModelPart * modelPart = m_partHash[moduleID];
	if (modelPart != NULL) return modelPart;

	if (m_referenceModel != NULL) {
		return m_referenceModel->retrieveModelPart(moduleID);
	}

	return NULL;
}

bool PaletteModel::containsModelPart(const QString & moduleID) {
	return m_partHash.contains(moduleID);
}

void PaletteModel::updateOrAddModelPart(const QString & moduleID, ModelPart *newOne) {
	ModelPart *oldOne = m_partHash[moduleID];
	if(oldOne) {
		oldOne->copy(newOne);
	} else {
		m_partHash.insert(moduleID, newOne);
	}
}

void PaletteModel::loadParts() {
	QStringList nameFilters;
	nameFilters << "*" + FritzingPartExtension << "*" + FritzingModuleExtension;

	JustAppendAllPartsInstances = true;   
	/// !!!!!!!!!!!!!!!!  "JustAppendAllPartsInstances = CreateAllPartsBinFile"
	/// !!!!!!!!!!!!!!!!  is incorrect
	/// !!!!!!!!!!!!!!!!  this flag was originally set up because sometimes we were appending a
	/// !!!!!!!!!!!!!!!!  single instance into an already existing file,
	/// !!!!!!!!!!!!!!!!  so simply appending new items as text gave us xml errors.
	/// !!!!!!!!!!!!!!!!  The problem was that there was no easy way to set the flag directly on the actual
	/// !!!!!!!!!!!!!!!!  function being used:  PaletteModel::LoadPart(), though maybe this deserves another look.
	/// !!!!!!!!!!!!!!!!  However, since we're starting from scratch in LoadParts, we can use the much faster 
	/// !!!!!!!!!!!!!!!!  file append method.  Since CreateAllPartsBinFile is false in release mode,
	/// !!!!!!!!!!!!!!!!  Fritzing was taking forever to start up.


	writeCommonBinsHeader();

	QDir * dir1 = FolderUtils::getApplicationSubFolder("parts");
	if (dir1 != NULL) {
		loadPartsAux(*dir1, nameFilters);
		delete dir1;
		dir1 = NULL;
	}

	QDir dir2(FolderUtils::getUserDataStorePath("parts"));
	loadPartsAux(dir2, nameFilters);

	dir1 = new QDir(":/resources/parts");
	loadPartsAux(*dir1, nameFilters);

	writeCommonBinsFooter();
	
	JustAppendAllPartsInstances = false;   
	/// !!!!!!!!!!!!!!!!  "JustAppendAllPartsInstances = !CreateAllPartsBinFile"
	/// !!!!!!!!!!!!!!!!  is incorrect
	/// !!!!!!!!!!!!!!!!  See above.  We simply want to restore the default, so that other functions calling
	/// !!!!!!!!!!!!!!!!  writeInstanceInCommonBin via LoadPart() will use the slower DomDocument methods,
	/// !!!!!!!!!!!!!!!!  since in that case we are appending to an already existing file.

	delete dir1;
}

void PaletteModel::writeCommonBinsHeader() {
	writeCommonBinsHeaderAux(CreateAllPartsBinFile, AllPartsBinFilePath, "All Parts");
	writeCommonBinsHeaderAux(CreateNonCorePartsBinFile, NonCorePartsBinFilePath, "All my parts");
}

void PaletteModel::writeCommonBinsHeaderAux(bool &doIt, const QString &filename, const QString &binName) {
	QString header =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"+
		QString("<module fritzingVersion='%1'>\n").arg(Version::versionString())+
		QString("\t<title>%1</title>\n").arg(binName)+
		"\t<instances>\n";
	writeToCommonBinAux(header, QFile::WriteOnly, doIt, filename);
}

void PaletteModel::writeCommonBinsFooter() {
	writeCommonBinsFooterAux(CreateAllPartsBinFile, AllPartsBinFilePath);
	writeCommonBinsFooterAux(CreateNonCorePartsBinFile, NonCorePartsBinFilePath);
}

void PaletteModel::writeCommonBinsFooterAux(bool &doIt, const QString &filename) {
	QString footer = "\t</instances>\n</module>\n";
	writeToCommonBinAux(footer, QFile::Append, doIt, filename);
}

void PaletteModel::writeInstanceInCommonBin(const QString &moduleID, const QString &path, bool &doIt, const QString &filename) {
	QString pathAux = path;
	pathAux.remove(FolderUtils::getApplicationSubFolderPath("")+"/");

	if (JustAppendAllPartsInstances) {
		QString instance = InstanceTemplate.arg(moduleID).arg(pathAux);
		writeToCommonBinAux(instance, QFile::Append, doIt, filename);
	}
	else {
		QString errorStr;
		int errorLine;
		int errorColumn;
		QDomDocument domDocument;

		QFile file(AllPartsBinFilePath);

		if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
			return;
		}

		QDomElement root = domDocument.documentElement();
   		if (root.isNull()) {
   			return;
		}

		if (root.tagName() != "module") {
			return;
		}

		QDomElement instances = root.firstChildElement("instances");
		if (instances.isNull()) return;

		QDomElement instance = domDocument.createElement("instance");
		instances.appendChild(instance);
		instance.setAttribute("moduleIdRef", moduleID);
		instance.setAttribute("path", pathAux);
		QDomElement views = domDocument.createElement("views");
		instance.appendChild(views);
		QDomElement iconView = domDocument.createElement("iconView");
		views.appendChild(iconView);
		iconView.setAttribute("layer", "icon");
		QDomElement geometry = domDocument.createElement("geometry");
		iconView.appendChild(geometry);
		geometry.setAttribute("x", "-1");
		geometry.setAttribute("y", "-1");
		geometry.setAttribute("z", "-1");
		writeToCommonBinAux(domDocument.toString(), QFile::WriteOnly, doIt, filename);
	}
}

void PaletteModel::writeToCommonBinAux(const QString &textToWrite, QIODevice::OpenMode openMode, bool &doIt, const QString &filename) {
	if(!doIt) return;

	QFile file(filename);
	if (!file.open(openMode | QFile::Text)) {
		doIt = false;
	} else {
		QTextStream out(&file);
		out << textToWrite;
		file.close();
	}
}

void PaletteModel::loadPartsAux(QDir & dir, QStringList & nameFilters) {
    QString temp = dir.absolutePath();
    QFileInfoList list = dir.entryInfoList(nameFilters, QDir::Files | QDir::NoSymLinks);
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString path = fileInfo.absoluteFilePath ();
        // DebugDialog::debug(QString("part path:%1 core? %2").arg(path).arg(m_loadingCore? "true" : "false"));
        loadPart(path);
    }

    QStringList dirs = dir.entryList(QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (int i = 0; i < dirs.size(); ++i) {
    	QString temp2 = dirs[i];
       	dir.cd(temp2);

       	//if(temp2 == "core" || temp2=="contrib" || temp2=="user") {
			m_loadingCore = temp2=="core";
			m_loadingContrib = temp2=="contrib";
       	//}

    	loadPartsAux(dir, nameFilters);
    	dir.cdUp();
    }
}

ModelPart * PaletteModel::loadPart(const QString & path, bool update) {
    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, QObject::tr("Fritzing"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(path)
                             .arg(file.errorString()));
        return NULL;
    }


	//DebugDialog::debug(QString("loading %1 %2").arg(path).arg(QTime::currentTime().toString("HH:mm:ss")));
    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument* domDocument = new QDomDocument();

    if (!domDocument->setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::information(NULL, QObject::tr("Fritzing"),
							 QObject::tr("Parse error (2) at line %1, column %2:\n%3\n%4")
							 .arg(errorLine)
							 .arg(errorColumn)
							 .arg(errorStr)
							 .arg(path));
        return NULL;
    }

    QDomElement root = domDocument->documentElement();
   	if (root.isNull()) {
        //QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (8)."));
   		return NULL;
	}

    if (root.tagName() != "module") {
        //QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (9)."));
        return NULL;
    }

	QString moduleID = root.attribute("moduleId");
	if (moduleID.isNull() || moduleID.isEmpty()) {
		//QMessageBox::information(NULL, QObject::tr("Fritzing"), QObject::tr("The file is not a Fritzing file (10)."));
		return NULL;
	}

    ModelPart::ItemType type = ModelPart::Part;
    // check if it's a wire
	QDomElement properties = root.firstChildElement("properties");
	QString propertiesText = properties.text();
	// FIXME: properties is nested right now
	if (propertiesText.contains("wire", Qt::CaseInsensitive)) {
		type = ModelPart::Wire;
	}
	else if (moduleID.compare(ModuleIDNames::jumperModuleIDName) == 0) {
		type = ModelPart::Jumper;
	}
	else if (propertiesText.contains("breadboard", Qt::CaseInsensitive)) {
		type = ModelPart::Breadboard;
	}
	else if (propertiesText.contains("plain vanilla pcb", Qt::CaseInsensitive)) {
		type = ModelPart::ResizableBoard;
	}
	else if (moduleID.compare(ModuleIDNames::groundPlaneModuleIDName) == 0) {
		type = ModelPart::CopperFill;
	}
	else if (propertiesText.contains("arduino", Qt::CaseInsensitive)) {
		LayerAttributes la;
		if (la.getSvgElementID(domDocument, ViewIdentifierClass::PCBView, ViewLayer::Board)) {
			type = ModelPart::Board;
		}
	}
	else if (propertiesText.contains("note", Qt::CaseInsensitive)) {
		type = ModelPart::Note;
	}
	/*else if (propertiesText.equals("module", Qt::CaseInsensitive)) {
		type = ModelPart::Module;
	}*/
	else if (propertiesText.contains("ground symbol", Qt::CaseInsensitive)) {
		type = ModelPart::Symbol;
	}
	else if (propertiesText.contains("power symbol", Qt::CaseInsensitive)) {
		type = ModelPart::Symbol;
	}
	ModelPart * modelPart = new ModelPart(domDocument, path, type);
	if (modelPart == NULL) return NULL;

	modelPart->setCore(m_loadingCore);
	modelPart->setContrib(m_loadingContrib);

	if (m_partHash.contains(moduleID) && m_partHash[moduleID]) {
		if(!update) {
			QMessageBox::warning(NULL, QObject::tr("Fritzing"),
							 QObject::tr("The part '%1' at '%2' does not have a unique module id '%3'.")
							 .arg(modelPart->title())
							 .arg(path)
							 .arg(moduleID));
			return NULL;
		} else {
			m_partHash[moduleID]->copyStuff(modelPart);
		}
	} else {
		m_partHash.insert(moduleID, modelPart);
	}

    if (m_root == NULL) {
		 m_root = modelPart;
	}
	else {
    	modelPart->setParent(m_root);
   	}

	//DebugDialog::debug(QString("all parts %1").arg(JustAppendAllPartsInstances));
    writeInstanceInCommonBin(moduleID,path,CreateAllPartsBinFile,AllPartsBinFilePath);

    bool keepOnCreatingNonCorePartBins = !modelPart->isCore();
	//DebugDialog::debug(QString("non core parts %1").arg(JustAppendAllPartsInstances));
    writeInstanceInCommonBin(moduleID,path,keepOnCreatingNonCorePartBins,NonCorePartsBinFilePath);
    if(!modelPart->isCore()) {
    	CreateNonCorePartsBinFile = keepOnCreatingNonCorePartBins;
    }
    
    return modelPart;
}

bool PaletteModel::load(const QString & fileName, ModelBase * refModel) {
	QList<ModelPart *> modelParts;
	bool result = ModelBase::load(fileName, refModel, modelParts);
	if (result) {
		m_loadedFromFile = true;
		m_loadedFrom = fileName;
	}
	return result;
}

bool PaletteModel::loadedFromFile() {
	return m_loadedFromFile;
}

QString PaletteModel::loadedFrom() {
	if(m_loadedFromFile) {
		return m_loadedFrom;
	} else {
		return ___emptyString___;
	}
}

ModelPart * PaletteModel::addPart(QString newPartPath, bool addToReference, bool updateIdAlreadyExists) {
	/*ModelPart * modelPart = loadPart(newPartPath, updateIdAlreadyExists);;
	if (m_referenceModel != NULL && addToReference) {
		modelPart = m_referenceModel->addPart(newPartPath, addToReference);
		if (modelPart != NULL) {
			return addModelPart( m_root, modelPart);
		}
	}*/

	ModelPart * modelPart = loadPart(newPartPath, updateIdAlreadyExists);
	if (m_referenceModel && addToReference) {
		m_referenceModel->addPart(modelPart,updateIdAlreadyExists);
	}

	return modelPart;
}

void PaletteModel::removePart(const QString &moduleID) {
	ModelPart *mpToRemove = NULL;
	QList<QObject *>::const_iterator i;
    for (i = m_root->children().constBegin(); i != m_root->children().constEnd(); ++i) {
		ModelPart* mp = qobject_cast<ModelPart *>(*i);
		if (mp == NULL) continue;

		if(mp->moduleID() == moduleID) {
			mpToRemove = mp;
			break;
		}
	}
	if(mpToRemove) {
		mpToRemove->setParent(NULL);
		delete mpToRemove;
	}
}

void PaletteModel::clearPartHash() {
	foreach (ModelPart * modelPart, m_partHash.values()) {
		delete modelPart;
	}
	m_partHash.clear();
}

void PaletteModel::setOrdererChildren(QList<QObject*> children) {
	m_root->setOrderedChildren(children);
}