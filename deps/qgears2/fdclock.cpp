/*
 *    hardinfo2 - System Information and Benchmark
 *    Copyright (C) 2024 hardinfo2 project
 *    License: GPL2+
 *
 *    Based on qgears2 by Zack Rusin (Public Domain)
 *    Based on cairogears by David Reveman & Peter Nilsson (Public Domain)
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "fdclock.h"

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QPaintEngine>

#include <QDebug>
#include <QTime>

#include <math.h>


#ifndef M_SQRT2
    #define M_SQRT2 1.41421356237309504880
#endif

static void drawArc(QPainterPath &path, double xc, double yc,
                    double radius, double angle2, double angle1)
{
    double xs = xc - radius;
    double ys = yc - radius;
    double width  = radius*2;
    double height = radius*2;
    double span = angle2 - angle1;

    path.arcTo(xs, ys, width, height, angle1, span);
}

static QPainterPath drawBoundary()
{
    QPainterPath path;
    path.moveTo(63.000,  36.000);

    path.cubicTo(63.000,  43.000,
                  58.000,  47.000,
                  51.000,  47.000);

    path.lineTo(13.000,  47.000);

    path.cubicTo(6.000,  47.000,
                  1.000,  43.000,
                  1.000,  36.000);

    path.lineTo(1.000,  12.000);

    path.cubicTo(1.000,  5.000,
                  6.000,  1.000,
                  13.000,  1.000);

    path.lineTo(51.000,  1.000);

    path.cubicTo(58.000,  1.000,
                  63.000,  5.000,
                  63.000,  12.000);
    path.closeSubpath();

    return path;
}

static void drawOutline(QPainter *p)
{
    QPen pen(QColor(255, 255, 255));
    pen.setWidthF(2.0);
    QPainterPath path = drawBoundary();
    p->strokePath(path, pen);
}

static void drawBackground(QPainter *p)
{
    p->save();
    QBrush brush(QColor(59, 128, 174));
    p->translate(3.5, 3.5);
    p->scale(0.887, 0.848);
    QPainterPath path = drawBoundary();
    p->fillPath(path, brush);
    p->restore();
}

static QPainterPath drawWindow()
{
    QPainterPath path;

    path.moveTo(-6.00, -7.125);

    path.lineTo(6.00, -7.125);

    path.cubicTo(8.00, -7.125,
                 9.00, -6.125,
                 9.00, -4.125);

    path.lineTo(9.00,  4.125);

    path.cubicTo(9.00,  6.125,
                 8.00,  7.125,
                 6.00,  7.125);

    path.lineTo(-6.00,  7.125);

    path.cubicTo(-8.00,  7.125,
                 -9.00,  6.125,
                 -9.00,  4.125);

    path.lineTo(-9.00, -4.125);

    path.cubicTo(-9.00, -6.125,
                 -8.00, -7.125,
                 -6.00, -7.125);
    path.closeSubpath();
    return path;
}

static void drawWindowAt(QPainter *p, double x, double y, double scale)
{
    p->save();
    {
	p->translate(x, y);
	p->scale(scale, scale);
	QPainterPath windowPath = drawWindow();
        QBrush brush(QColor(255, 255, 255));
        p->fillPath(windowPath, brush);
        QPen pen(QColor(59, 128, 174));
	p->scale(1/scale, 1/scale);
	p->strokePath(windowPath, pen);
    }
    p->restore();
}

void drawWindows(QPainter *p)
{
    {
        QPainterPath path;
	path.moveTo(18.00, 16.125);
	path.lineTo(48.25, 20.375);
	path.lineTo(30.25, 35.825);
	path.closeSubpath();
        QPen pen(QColor(255, 255, 255, 127));
	p->strokePath(path, pen);
    }
    drawWindowAt(p, 18.00, 16.125, 1);
    drawWindowAt(p, 48.25, 20.375, 0.8);
    drawWindowAt(p, 30.25, 35.825, 0.5);
}

#define FDLOGO_ROT_X_FACTOR	1.086
#define FDLOGO_ROT_Y_FACTOR	1.213
#define FDLOGO_WIDTH		(64 * FDLOGO_ROT_X_FACTOR)
#define FDLOGO_HEIGHT		(48 * FDLOGO_ROT_Y_FACTOR)

static void drawFancyTick(QPainter *p, double radius)
{
    QPainterPath path1;
    drawArc(path1, 0, 0, radius, 0, 360);
    p->fillPath(path1, QColor(59, 128, 174));

    QPen pen(QColor(136, 136, 136));
    pen.setWidthF(radius * 2 / 3);
    QPainterPath path2;
    path2.moveTo(0+2*radius, 0);
    drawArc(path2, 0, 0, radius * 2, 0, -360);
    p->strokePath(path2, pen);
}

static void drawPlainTick(QPainter *p, double radius)
{
    QPainterPath path;
    drawArc(path, 0, 0, radius, 0, 360);
    p->fillPath(path, QColor(59, 128, 173));
}

void FdClock::faceDraw(QPainter *p, double width, double height)
{
    int minute;

    p->save();
    {
        p->scale(width, height);
	p->save();
	{
	    p->translate(.15, .15);
	    p->scale(.7, .7);
	    logoDraw(p, 1, 1);
	}
	p->restore();
	p->translate(0.5, 0.5);
	p->scale(0.93, 0.93);
	for (minute = 0; minute < 60; minute++)
	{
	    double  degrees;
	    p->save();
	    degrees = minute * 6.0;
	    p->rotate(degrees);
	    p->translate(0, 0.5);
	    if (minute % 15 == 0)
		drawFancyTick(p, 0.015);
	    else if (minute % 5 == 0)
		drawFancyTick(p, 0.01);
	    else
		drawPlainTick(p, 0.01);
	    p->restore();
	}
    }
    p->restore();
}


void
FdClock::logoDraw(QPainter *p, double width, double height)
{
    double  x_scale, y_scale, scale, x_off, y_off;
    p->save();
    x_scale = width / FDLOGO_WIDTH;
    y_scale = height / FDLOGO_HEIGHT;
    scale = x_scale < y_scale ? x_scale : y_scale;
    x_off = (width - (scale * FDLOGO_WIDTH)) / 2;
    y_off = (height - (scale * FDLOGO_HEIGHT)) / 2;
    p->translate(x_off, y_off);
    p->scale(scale, scale);

    p->translate(-2.5, 14.75);
    p->rotate(-16);

    drawOutline(p);
    drawBackground(p);
    drawWindows(p);
    p->restore();
}

static QPainterPath drawHour(double width, double length)
{
    QPainterPath path;

    double  r = width / 2;
    path.moveTo(length, -r);
    drawArc(path, length, 0, r, -90, 90);
    path.lineTo (width * M_SQRT2, r);
    drawArc(path, 0, 0, width, 30, 330);
    path.closeSubpath();
    return path;
}

static QPainterPath drawMinute(double width, double length)
{
    QPainterPath path;
    double  r = width / 2;
    path.moveTo(length, -r);
    drawArc(path, length, 0, r, -90, 90);
    path.lineTo(0, r);
    path.lineTo(0, -r);
    path.closeSubpath();
    return path;
}

static QPainterPath drawSecond(double width, double length)
{
    double  r = width / 2;
    double  thick = width;
    double  back = length / 3;
    double  back_thin = length / 10;

    QPainterPath path;
    path.moveTo(length,     -r);
    drawArc    (path,	length, 0, r, -90, 90);
    path.lineTo(-back_thin,  r);
    path.lineTo(-back_thin,  thick);
    path.lineTo(-back,       thick);
    path.lineTo(-back,      -thick);
    path.lineTo(-back_thin, -thick);
    path.lineTo(-back_thin, -r);
    path.closeSubpath();

    return path;
}

static void drawHand(QPainter *p, double angle, double width, double length, double alt,
                     QPainterPath (*draw)(double width, double length))
{
    p->save();
    {//shadow
        p->translate(alt/2, alt);
        p->rotate(angle);
	QPainterPath path = (*draw)(width, length);
        p->fillPath(path, QColor(0, 0, 0, 77));
    }
    p->restore();

    p->save();
    {
	p->rotate(angle);
	QPainterPath path = (*draw)(width, length);
        p->fillPath(path, QColor(0, 0, 0));
    }
    p->restore();
}

static void drawTime(QPainter *p, double width, double height, int seconds)
{
    QTime qtm = QTime::currentTime();
    double  hour_angle, minute_angle, second_angle;

    second_angle = (qtm.second() + qtm.msec() / 1000.0) * 6.0;
    minute_angle = qtm.minute() * 6.0 + second_angle / 60.0;
    hour_angle = qtm.hour() * 30.0 + minute_angle / 12.0;

    p->save();
    {
	p->scale(width, height);
	p->translate(0.5, 0.5);
	drawHand(p, hour_angle - 90,
                 0.03, 0.25, 0.010, drawHour);
	drawHand(p, minute_angle - 90,
                 0.015, 0.39, 0.020, drawMinute);
	if (seconds)
	    drawHand(p, second_angle - 90,
                     0.0075, 0.32, 0.026, drawSecond);
    }
    p->restore();
}

void FdClock::handDraw(QPainter *p, double width, double height,
                       int seconds)
{
    drawTime(p, width, height, seconds);
}

void FdClock::render(QPainter *p, double width, double height)
{
    //p->setCompositionMode(QPainter::CompositionMode_Source);
    //p->fillRect(0, 0, width, height,  Qt::white);
    if (p->paintEngine()->hasFeature(QPaintEngine::PorterDuff))
        p->setCompositionMode(QPainter::CompositionMode_SourceOver);
    p->save();
    QBrush brush(QColor(255, 255, 255, 64));
    p->scale(width, height);
    p->translate(0.5, 0.5);
    QPainterPath path;
    drawArc(path, 0, 0, 0.5, 0, 360);
    p->fillPath(path, brush);
    p->restore();

    faceDraw(p, width, height);

    handDraw(p, width, height, 1);
}
