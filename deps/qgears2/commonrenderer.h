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

#ifndef COMMONRENDERER_H
#define COMMONRENDERER_H

class QPainter;
class QWidget;
class QImage;
class QPixmap;

#include <QTime>
#include <QPainterPath>

#define NUMPTS 12

class CommonRenderer
{
public:
    enum Mode
    {
        GEARSFANCY,
        GEARS
    };
public:
    CommonRenderer();
    virtual ~CommonRenderer();

    void setMode(Mode mode);
protected:
    virtual void renderTo(QPaintDevice *device);

    void gears(QPaintDevice *device);

    QPainterPath gearPath(double inner_radius, double outer_radius,
                          int teeth, double tooth_depth) const;
    void pathRender(QPainter *p, QPaintDevice *device);
    void gearsRender(QPainter *p);

    void setup(int w, int h);
    void animate(double *pts, double *deltas,
                 int index, int limit);
    void animateStep(int w, int h);
    void printFrameRate();

    QPainterPath m_gear1;
    QPainterPath m_gear2;
    QPainterPath m_gear3;

    bool setupFinished;

    qreal animpts[NUMPTS * 2];
    qreal deltas[NUMPTS * 2];

    Mode mode;

    QTime ptime;
    unsigned int frame_cnt;
};

#endif
