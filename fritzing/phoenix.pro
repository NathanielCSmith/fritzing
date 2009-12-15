# /*******************************************************************
#
# Part of the Fritzing project - http://fritzing.org
# Copyright (c) 2007-08 Fachhochschule Potsdam - http://fh-potsdam.de
#
# Fritzing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Fritzing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.
#
# ********************************************************************
#
# $Revision$:
# $Author$:
# $Date$
#
#********************************************************************/

CONFIG += debug_and_release
win32 {
	CONFIG -= embed_manifest_exe
	INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib
	DEFINES += _CRT_SECURE_NO_DEPRECATE
}
macx {
	CONFIG += x86 ppc
	QMAKE_INFO_PLIST = FritzingInfo.plist
	DEFINES += QT_NO_DEBUG
        LIBS += /usr/lib/libz.dylib
}
unix {
    !macx {						# unix is defined on mac
        HARDWARE_PLATFORM = $$system(uname -m)
        contains( HARDWARE_PLATFORM, x86_64 ) {
            DEFINES += LINUX_64
        } else {
            DEFINES += LINUX_32
		}
    }
}
ICON = resources/images/fritzing_icon.icns
QT += core \
    gui \
    svg \
    xml \
    network \
    webkit \
    sql
RC_FILE = fritzing.rc
RESOURCES += phoenixresources.qrc
	include(pri/kitchensink.pri)
	include(pri/quazip.pri)
	include(pri/partsbinpalette.pri)
	include(pri/partseditor.pri)
	include(pri/referencemodel.pri)
	include(pri/svg.pri)
	include(pri/help.pri)
	include(pri/labels.pri)
	include(pri/itemselection.pri)
	include(pri/version.pri)
	include(pri/group.pri)
	include(pri/eagle.pri)
	include(pri/utils.pri)
	include(pri/viewswitcher.pri)
	include(pri/navigator.pri)
	include(pri/items.pri)
	include(pri/autoroute.pri)
	include(pri/dialogs.pri)
	include(pri/connectors.pri)
	include(pri/infoview.pri)
	include(pri/model.pri)
	include(pri/sketch.pri)
TARGET = Fritzing
TEMPLATE = app
TRANSLATIONS += translations/fritzing_de.ts \
	translations/fritzing_fr.ts \
	translations/fritzing_en.ts \
	translations/fritzing_es.ts \
	translations/fritzing_ja.ts \
	translations/fritzing_pt_br.ts \
	translations/fritzing_pt_pt.ts \
	translations/fritzing_it.ts \
	translations/fritzing_ru.ts \
	translations/fritzing_zh.ts \
	translations/fritzing_et.ts \
	translations/fritzing_hu.ts \
	translations/fritzing_th.ts \
	translations/fritzing_nl.ts
	
