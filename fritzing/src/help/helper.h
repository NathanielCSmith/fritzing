/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-08 Fachhochschule Potsdam - http://fh-potsdam.de

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

/*
 * Class that manage all the interactive helps in the main window
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <QObject>

#include "sketchmainhelp.h"
#include "toolhelp.h"
#include "../mainwindow.h"

class Helper : public QObject {
	Q_OBJECT
	public:
		Helper(MainWindow *owner);

	protected slots:
		void init();
		void viewChanged();
		void somethingDroppedIntoView();
		void viewSwitched();
		void autorouted();

	protected:
		void addItemToView(QGraphicsWidget *item, SketchWidget* view);
		void centerItemInView(SketchMainHelp *item, SketchWidget* view);
		void fixedX(ToolHelp *item, SketchWidget* view);
		void fixedY(ToolHelp *item, SketchWidget* view);
		void moveItemBy(QGraphicsProxyWidget *item, qreal dx, qreal dy);

	protected:
		MainWindow *m_owner;

		SketchMainHelp *m_breadMainHelp;
		SketchMainHelp *m_schemMainHelp;
		SketchMainHelp *m_pcbMainHelp;

		ToolHelp *m_partsBinHelp;
		ToolHelp *m_autorouteHelp;
		ToolHelp *m_switchButtonsHelp;

		bool m_stillWaitingFirstDrop;
		bool m_stillWaitingFirstViewSwitch;
		bool m_stillWaitingFirstAutoroute;

	protected:
		static QString BreadboardHelpText;
		static QString SchematicHelpText;
		static QString PCBHelpText;

		static QString PartsBinHelpText;
		static QString AutorouteHelpText;
		static QString SwitchButtonsHelpText;
};

#endif /* HELPER_H_ */
