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

$Revision: 1558 $:
$Author: cohen@irascible.com $:
$Date: 2008-11-19 12:11:07 +0100 (Wed, 19 Nov 2008) $

********************************************************************/




#ifndef SKETCHTOOLBUTTON_H_
#define SKETCHTOOLBUTTON_H_

#include <QToolButton>

class SketchToolButton : public QToolButton {
public:
	SketchToolButton(QWidget *parent=0);

protected:
	void actionEvent(QActionEvent *event);
};

#endif /* SKETCHTOOLBUTTON_H_ */
