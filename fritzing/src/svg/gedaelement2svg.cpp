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

#include "gedaelement2svg.h"
#include "gedaelementparser.h"
#include "gedaelementlexer.h"
#include "../utils/textutils.h"
#include "svgfilesplitter.h"

#include <QFile>
#include <QTextStream>
#include <QObject>
#include <limits>
#include <QDomDocument>
#include <QDomElement>
#include <qmath.h>

static const int MAX_INT = std::numeric_limits<int>::max();
static const int MIN_INT = std::numeric_limits<int>::min();

GedaElement2Svg::GedaElement2Svg() {
}

QString GedaElement2Svg::convert(QString filename) 
{
	QFile file(filename);
	if (!file.open(QFile::ReadOnly)) {
		throw QObject::tr("unable to open %1").arg(filename);
	}

	QString text;
	QTextStream textStream(&file);
	text = textStream.readAll();

	GedaElementLexer lexer(text);
	GedaElementParser parser;

	if (!parser.parse(&lexer)) {
		throw QObject::tr("unable to parse %1").arg(filename);
	}

	// TODO: other layers
	QString copper0;
	QString copper1;
	QString silkscreen;

	m_maxX = MIN_INT;
	m_maxY = MIN_INT;
	m_minX = MAX_INT;
	m_minY = MAX_INT;
	m_nameList.clear();
	m_numberList.clear();
	QVector<QVariant> stack = parser.symStack();

	QStringList already;
	for (int ix = 0; ix < stack.size(); ) { 
		QVariant var = stack[ix];
		if (var.type() == QVariant::String) {
			QString thing = var.toString();
			int argCount = countArgs(stack, ix);
			bool mils = stack[ix + argCount + 1].toChar() == ')';
			if (thing.compare("element", Qt::CaseInsensitive) == 0) {
			}
			else if (thing.compare("pad", Qt::CaseInsensitive) == 0) {
				QString s = convertPad(stack, ix, argCount, mils);
				if (!already.contains(s)) {
					copper1 += s;
					already.append(s);
				}
			}
			else if (thing.compare("pin", Qt::CaseInsensitive) == 0) {
				QString s = convertPin(stack, ix, argCount, mils);
				if (!already.contains(s)) {
					copper0 += s;
					already.append(s);
				}
			}
			else if (thing.compare("elementline", Qt::CaseInsensitive) == 0) {
				silkscreen += convertPad(stack, ix, argCount, mils);
			}
			else if (thing.compare("elementarc", Qt::CaseInsensitive) == 0) {
				silkscreen += convertArc(stack, ix, argCount, mils);
			}
			else if (thing.compare("mark", Qt::CaseInsensitive) == 0) {
			}
			ix += argCount + 2;
		}
		else if (var.type() == QVariant::Char) {
			// will arrive here at the end of the element
			// TODO: shouldn't happen otherwise
			ix++;
		}
		else {
			throw QObject::tr("parse failure in %1").arg(filename);
		}
	}

	// TODO: offset everything if minx or miny < 0
	copper0 = offsetMin("<g id='copper0'>" + copper0 + "</g>");
	copper1 = offsetMin("<g id='copper1'>" + copper1 + "</g>");
	silkscreen = offsetMin("<g id='silkscreen'>" + silkscreen + "</g>");

	QString svg = TextUtils::makeSVGHeader(100000, 100000, m_maxX - m_minX, m_maxY - m_minY) + copper0 + copper1 + silkscreen + "</svg>";

	return svg;
}

QString GedaElement2Svg::offsetMin(const QString & svg) {
	if (m_minX == 0 && m_minY == 0) return svg;

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;
	if (!domDocument.setContent(svg, true, &errorStr, &errorLine, &errorColumn)) {
		throw QObject::tr("failure in svg conversion 1: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn);
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		throw QObject::tr("failure in svg conversion 2: %1 %2 %3").arg(errorStr).arg(errorLine).arg(errorColumn);
	}

	SvgFileSplitter splitter;
	splitter.shiftChild(root, -m_minX, -m_minY);
	return TextUtils::removeXMLEntities(domDocument.toString());
}

int GedaElement2Svg::countArgs(QVector<QVariant> & stack, int ix) {
	int argCount = 0;
	for (int i = ix + 1; i < stack.size(); i++) {
		QVariant var = stack[i];
		if (var.type() == QVariant::Char) {
			QChar ch = var.toChar();
			if (ch == ']' || ch == ')') {
				break;
			}
		}

		argCount++;
	}

	return argCount;
}

QString GedaElement2Svg::convertPin(QVector<QVariant> & stack, int ix, int argCount, bool mils)
{
	qreal drill = 0;
	QString name;
	QString number;

	//int flags = stack[ix + argCount].toInt();
	//bool useNumber = (flags & 1) != 0;

	if (argCount == 9) {
		drill = stack[ix + 6].toInt();
		name = stack[ix + 7].toString();
		number = stack[ix + 8].toString();
	}
	else if (argCount == 7) {
		drill = stack[ix + 4].toInt();
		name = stack[ix + 5].toString();
		number = stack[ix + 6].toString();
	}
	else if (argCount == 6) {
		drill = stack[ix + 4].toInt();
		name = stack[ix + 5].toString();
	}
	else if (argCount == 5) {
		name = stack[ix + 4].toString();
	}
	else {
		throw QObject::tr("bad pin argument count");
	}

	bool repeat;
	QString pinID = getPinID(number, name, repeat);
	if (repeat) {
		// TODO:  increment the id...
	}

	int cx = stack[ix + 1].toInt();
	int cy = stack[ix + 2].toInt();
	qreal r = stack[ix + 3].toInt() / 2.0;
	drill /= 2.0;

	if (mils) {
		// lo res
		cx *= 100;
		cy *= 100;
		r *= 100;
		drill *= 100;
	}

	if (cx - r < m_minX) m_minX = cx - r;
	if (cx + r > m_maxX) m_maxX = cx + r;
	if (cy - r < m_minY) m_minY = cy - r;
	if (cy + r > m_maxY) m_maxY = cy + r;

	qreal w = r - drill;

	// TODO: what if multiple pins have the same id--need to clear or increment the other ids. also put the pins on a bus?
	// TODO:  if the pin has a name, post it up to the fz as the connector name


	QString circle = QString("<circle fill='none' cx='%1' cy='%2' stroke='rgb(255, 191, 0)' r='%3' id='%4' connectorname='%5' stroke-width='%6' />")
					.arg(cx)
					.arg(cy)
					.arg(r - (w / 2))
					.arg(pinID)
					.arg(name)
					.arg(w);
	return circle;
}

QString GedaElement2Svg::convertPad(QVector<QVariant> & stack, int ix, int argCount, bool mils)
{
	QString name; 
	QString number;

	int flags = (argCount > 5) ? stack[ix + argCount].toInt() : 0;
	bool square = (flags & 0x0100) != 0;
	int x1 = stack[ix + 1].toInt();
	int y1 = stack[ix + 2].toInt();
	int x2 = stack[ix + 3].toInt();
	int y2 = stack[ix + 4].toInt();
	int thickness = stack[ix + 5].toInt();

	bool isPad = true;
	if (argCount == 10) {
		name = stack[ix + 8].toString();
		number = stack[ix + 9].toString();
		QString sflags = stack[ix + argCount].toString();
		if (sflags.contains("square", Qt::CaseInsensitive)) {
			square = true;
		}
	}
	else if (argCount == 8) {
		name = stack[ix + 6].toString();
		number = stack[ix + 7].toString();
	}
	else if (argCount == 7) {
		name = stack[ix + 6].toString();
	}
	else if (argCount == 5) {
		// this is an elementline
		isPad = false;
	}
	else {
		throw QObject::tr("bad pad argument count");
	}

	QString pinID;
	if (isPad) {
		bool repeat;
		pinID = getPinID(number, name, repeat);
		if (repeat) {
			// TODO: increment id number
		}
	}

	if (mils) {
		// lo res
		x1 *= 100;
		y1 *= 100;
		x2 *= 100;
		y2 *= 100;
		thickness *= 100;
	}

	qreal halft = thickness / 2.0;

	// don't know which of the coordinates is larger so check them all
	if (x1 - halft < m_minX) m_minX = x1 - halft;
	if (x2 - halft < m_minX) m_minX = x2 - halft;
	if (x1 + halft > m_maxX) m_maxX = x1 + halft;
	if (x2 + halft > m_maxX) m_maxX = x2 + halft;
	if (y1 - halft < m_minY) m_minY = y1 - halft;
	if (y2 - halft < m_minY) m_minY = y2 - halft;
	if (y1 + halft > m_maxY) m_maxY = y1 + halft;
	if (y2 + halft > m_maxY) m_maxY = y2 + halft;
	  
	QString line = QString("<line fill='none' x1='%1' y1='%2' x2='%3' y2='%4' stroke-width='%5' ")
					.arg(x1)
					.arg(y1)
					.arg(x2)
					.arg(y2)
					.arg(thickness);
	if (!isPad) {
		// elementline
		line += "stroke='white' ";
	}
	else {
		line += QString("stroke='rgb(255, 148, 0)' stroke-linecap='%1' stroke-linejoin='%2' id='%3' connectorname='%4' ")
					.arg(square ? "square" : "round")
					.arg(square ? "miter" : "round")
					.arg(pinID)
					.arg(name);
	}

	line += "/>";
	return line;
}

QString GedaElement2Svg::convertArc(QVector<QVariant> & stack, int ix, int argCount, bool mils)
{
	Q_UNUSED(argCount);

	int x = stack[ix + 1].toInt();
	int y = stack[ix + 2].toInt();
	qreal w = stack[ix + 3].toInt();
	qreal h = stack[ix + 4].toInt();

	// In PCB, an angle of zero points left (negative X direction), and 90 degrees points down (positive Y direction)
	int startAngle = (stack[ix + 5].toInt()) + 180;
	//  Positive angles sweep counterclockwise
	int deltaAngle = stack[ix + 6].toInt();

	int thickness = stack[ix + 7].toInt();

	if (mils) {
		// lo res
		x *= 100;
		y *= 100;
		w *= 100;
		h *= 100;
		thickness *= 100;
	}

	qreal halft = thickness / 2.0;
	if (x - w - halft < m_minX) m_minX = x - w - halft;
	if (x + w + halft > m_maxX) m_maxX = x + w + halft;
	if (y - h - halft < m_minY) m_minY = y - h - halft;
	if (y + h + halft > m_maxY) m_maxY = y + h + halft;

	if (deltaAngle == 360) {
		if (w == h) {
			QString circle = QString("<circle fill='none' cx='%1' cy='%2' stroke='white' r='%3' stroke-width='%4' />")
							.arg(x)
							.arg(y)
							.arg(w)
							.arg(thickness);

			return circle;
		}

		QString ellipse = QString("<ellipse fill='none' cx='%1' cy='%2' stroke='white' rx='%3' ry='%4' stroke-width='%5' />")
						.arg(x)
						.arg(y)
						.arg(w)
						.arg(h)
						.arg(thickness);

		return ellipse;
	}

	int quad = 0;
	int startAngleQ1 = reflectQuad(startAngle, quad);
	qreal q = atan(w * tan(2 * M_PI * startAngleQ1 / 360.0) / h);
	qreal px = w * cos(q);
	qreal py = -h * sin(q);
	fixQuad(quad, px, py);
	int endAngleQ1 = reflectQuad(startAngle + deltaAngle, quad);
	q = atan(w * tan(2 * M_PI * endAngleQ1 / 360.0) / h);
	qreal qx = w * cos(q);
	qreal qy = -h * sin(q);
	fixQuad(quad, qx, qy);

	QString arc = QString("<path fill='none' stroke-width='%1' stroke='white' d='M%2,%3a%4,%5 0 %6,%7 %8,%9' />")
			.arg(thickness)
			.arg(px + x)
			.arg(py + y)
			.arg(w)
			.arg(h)
			.arg(qAbs(deltaAngle) >= 180 ? 1 : 0)
			.arg(deltaAngle > 0 ? 0 : 1)
			.arg(qx - px)
			.arg(qy - py);

	return arc;
}

QString GedaElement2Svg::unquote(const QString & string) {
	QString s = string;
	if (s.endsWith('"')) {
		s.chop(1);
	}
	if (s.startsWith('"')) {
		s = s.remove(0, 1);
	}

	return s;

}

void GedaElement2Svg::fixQuad(int quad, qreal & px, qreal & py) {
	switch (quad) {
		case 0:
			break;
		case 1:
			px = -px;
			break;
		case 2:
			px = -px;
			py = -py;
			break;
		case 3:
			py = -py;
			break;
	}
}

int GedaElement2Svg::reflectQuad(int angle, int & quad) {
	angle = angle %360;
	if (angle < 0) angle += 360;
	quad = angle / 90;
	switch (quad) {
		case 0:
			return angle;
		case 1:
			return 180 - angle;
		case 2:
			return angle - 180;
		case 3:
			return 360 - angle;
	}

	// never gets here, but keeps compiler happy
	return angle;
}

QString GedaElement2Svg::getPinID(QString & number, QString & name, bool & repeat) {

	repeat = false;

	if (!number.isEmpty()) {
		number = unquote(number);
	}
	if (!name.isEmpty()) {
		name = unquote(name);
	}

	if (!number.isEmpty()) {
		if (m_numberList.contains(number)) {
			repeat = true;
		}
		else {
			m_numberList.append(number);
		}

		bool ok;
		int n = number.toInt(&ok);
		return (ok ? QString("connector%1pin").arg(n - 1) : QString("connector%1pin").arg(number));
	}

	if (!name.isEmpty()) {
		if (m_nameList.contains(name)) {
			repeat = true;
		}
		else {
			m_nameList.append(name);
		}

		if (number.isEmpty()) {
			bool ok;
			int n = name.toInt(&ok);
			if (ok) {
				return QString("connector%1pin").arg(n - 1);
			}
		}
	}

	return "";
}
