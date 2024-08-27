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

#include "commonrenderer.h"

#include "fdclock.h"

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QBrush>
#include <QGradient>
#include <QImage>
#include <QPixmap>
#include <QRectF>
#include <QPaintEngine>

#include <QDebug>

#include <math.h>
#include <signal.h>

extern double drand()
{
    return (double)rand()/(double)RAND_MAX;
}

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

double gear1_rotation = 35;
double gear2_rotation = 24;
double gear3_rotation = 33.5;

#define LINEWIDTH 3

#define FILL_R 26
#define FILL_G 26
#define FILL_B 191
#define FILL_OPACITY 127

#define STROKE_R 26
#define STROKE_G 191
#define STROKE_B 26
#define STROKE_OPACITY 255

CommonRenderer::CommonRenderer(): setupFinished(false)
{
    m_gear1 = gearPath(2*30.0, 2*120.0, 50, 20.0);
    m_gear2 = gearPath(2*15.0, 2*75.0,  40, 20.0);
    m_gear3 = gearPath(2*20.0, 2*90.0,  30, 20.0);

    frame_cnt = 0;
    ptime = QTime::currentTime();
}

void CommonRenderer::renderTo(QPaintDevice *device)
{
    gears(device);

    ++frame_cnt;

    QTime ctime = QTime::currentTime();
    int ms = ptime.msecsTo(ctime);
    if (ms>=4000){
        printFrameRate();
        exit(0);
    }
}

QPainterPath CommonRenderer::gearPath(double inner_radius, double outer_radius,
                                      int teeth, double tooth_depth) const
{
    int i;
    double r0, r1, r2;
    double angle, da;

    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0;
    r2 = outer_radius + tooth_depth / 2.0;

    da = 2.0 * M_PI / (qreal) teeth / 4.0;

    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);

    angle = 0.0;
    path.moveTo(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da));

    for (i = 1; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / (qreal)teeth;

        path.lineTo(r1 * cos(angle), r1 * sin(angle));
        path.lineTo(r2 * cos(angle + da), r2 * sin(angle + da));
        path.lineTo(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da));

        if (i < teeth) path.lineTo(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da));
    }

    path.closeSubpath();

    path.moveTo(r0 * cos(angle + 3 * da), r0 * sin(angle + 3 * da));

    for (i = 1; i <= teeth; i++) {
        angle = i * 2.0 * M_PI / (qreal) teeth;

        path.lineTo(r0 * cos(angle), r0 * sin(angle));
    }

    path.closeSubpath();
    return path;
}

CommonRenderer::~CommonRenderer()
{

}

void CommonRenderer::setup(int w, int h)
{
    int i;

    for (i = 0; i < (NUMPTS * 2); i += 2) {
        animpts[i + 0] = (qreal) (drand() * w);
        animpts[i + 1] = (qreal) (drand() * h);
        deltas[i + 0] = (qreal) (drand() * 6.0 + 4.0);
        deltas[i + 1] = (qreal) (drand() * 6.0 + 4.0);
        if (animpts[i + 0] > w / 2.0) {
            deltas[i + 0] = -deltas[i + 0];
        }
        if (animpts[i + 1] > h / 2.0) {
            deltas[i + 1] = -deltas[i + 1];
        }
    }
    setupFinished = true;
}

void CommonRenderer::animate(qreal *pts, qreal *deltas, int index, int limit)
{
    qreal newpt = pts[index] + deltas[index];

    if (newpt <= 0) {
        newpt = -newpt;
        deltas[index] = (qreal) (drand() * 4.0 + 2.0);
    } else if (newpt >= (qreal) limit) {
        newpt = 2.0 * limit - newpt;
        deltas[index] = - (qreal) (drand() * 4.0 + 2.0);
    }
    pts[index] = newpt;
}

void CommonRenderer::animateStep(int w, int h)
{
    int i;

    for (i = 0; i < (NUMPTS * 2); i += 2) {
        animate(animpts, deltas, i + 0, w);
        animate(animpts, deltas, i + 1, h);
    }
}

void CommonRenderer::pathRender(QPainter *p, QPaintDevice *device)
{
    qreal *ctrlpts = animpts;
    int len = (NUMPTS * 2);
    qreal prevx = ctrlpts[len - 2];
    qreal prevy = ctrlpts[len - 1];
    qreal curx = ctrlpts[0];
    qreal cury = ctrlpts[1];
    qreal midx = (curx + prevx) / 2.0;
    qreal midy = (cury + prevy) / 2.0;

    animateStep(device->width(), device->height());

    QPainterPath path;
    path.moveTo(midx, midy);

    for (int i = 2; i <= (NUMPTS * 2); i += 2) {
        qreal x2, x1 = (midx + curx) / 2.0;
        qreal y2, y1 = (midy + cury) / 2.0;

        prevx = curx;
        prevy = cury;
        if (i < (NUMPTS * 2)) {
            curx = ctrlpts[i + 0];
            cury = ctrlpts[i + 1];
        } else {
            curx = ctrlpts[0];
            cury = ctrlpts[1];
        }
        midx = (curx + prevx) / 2.0;
        midy = (cury + prevy) / 2.0;
        x2 = (prevx + midx) / 2.0;
        y2 = (prevy + midy) / 2.0;
        path.cubicTo(x1, y1, x2, y2, midx, midy);
    }
    path.closeSubpath();

    if (mode == GEARSFANCY) {
        QRectF rect = path.controlPointRect();

        //qDebug()<<rect.topLeft()<<rect.bottomRight();
        QLinearGradient linearGrad(rect.topLeft(), rect.bottomRight());
        linearGrad.setColorAt(0, QColor(0, 0, 255, 191));
        linearGrad.setColorAt(1, QColor(255, 0, 0, 255));

        QBrush brush(linearGrad);
        p->setBrush(brush);
    } else {
        QBrush brush(QColor(FILL_R, FILL_G, FILL_B, FILL_OPACITY));
        p->setBrush(brush);
    }

    p->save();
    p->setBrush(QColor(0, 0, 0, 77));
    //p->setPen(QColor(0, 0, 0, 77));
    p->setPen(Qt::NoPen);
    p->translate(-10, -10);
    p->drawPath(path);
    p->restore();

    QPen pen(QColor(STROKE_R, STROKE_G, STROKE_B, STROKE_OPACITY));
    pen.setWidth(LINEWIDTH);
    p->setPen(pen);
    p->drawPath(path);
}

void CommonRenderer::gearsRender(QPainter *p)
{
    p->save();
    p->translate(2*170, 2*330.0);
    p->rotate(gear1_rotation);
    p->drawPath(m_gear1);
    p->restore();

    p->save();
    p->translate(2*369.0, 2*330.0);
    p->rotate(gear2_rotation);
    p->drawPath(m_gear2);
    p->restore();

    p->save();
    p->translate(2*170.0, 2*116.0);
    p->rotate(gear3_rotation);
    p->drawPath(m_gear3);
    p->restore();
}

void CommonRenderer::gears(QPaintDevice *device)
{
    if (!setupFinished)
        setup(device->width(), device->height());

    QPainter painter(device);
    painter.setRenderHint(QPainter::Antialiasing, true);

    //QLinearGradient gradient(QPoint(0, 0), QPoint(device->width(), device->height()));
    //gradient.setColorAt(0, QColor(127, 255, 0, 0));
    //gradient.setColorAt(1, QColor(127, 0, 255, 0));
    //QBrush diagonalGradient(gradient);
    //painter.setBrush(diagonalGradient);
    //painter.setBrush(QColor(255, 255, 0, 0));
    if (painter.paintEngine()->hasFeature(QPaintEngine::PorterDuff))
        painter.setCompositionMode(QPainter::CompositionMode_Source);
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        if (painter.paintEngine()->hasFeature(QPaintEngine::PorterDuff))
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        painter.save();
        //painter.setPen(QColor(0, 0, 0, 77));
        painter.setPen(Qt::NoPen); //no stroking
        painter.setBrush(QColor(0, 0, 0, 77));
        painter.translate(-10, -10);
        gearsRender(&painter);
        painter.restore();

        //painter.setBrush(QColor(191, 191, 191));
	QRectF rect = m_gear1.controlPointRect();
	QLinearGradient linearGrad(rect.topLeft(), rect.bottomRight());
	linearGrad.setColorAt(0, QColor(255, 0, 255, 191));
	linearGrad.setColorAt(1, QColor(0, 255, 255, 255));
	QBrush brush(linearGrad);
	painter.setBrush(brush);
        gearsRender(&painter);

        if (mode == GEARSFANCY) {
            static int CLOCK_W = 256*1.5;
            static int CLOCK_H = 245*1.5;
            static qreal xs = 1.0;
            static qreal ys = 1.0;
            static bool up = true;
            painter.save();
            painter.translate((painter.device()->width() -CLOCK_W) - (CLOCK_W/2)*xs,
                              (painter.device()->height()-CLOCK_H) - (CLOCK_H/2)*ys);
            painter.scale(xs, ys);
            FdClock::render(&painter, CLOCK_W, CLOCK_H);
            if (xs >= 2 || ys >= 2) {
                up = false;
            }
            else if (xs <= 1 || ys <= 1) {
                up = true;
            }
            qreal diff = 0.01;
            if (!up)
                diff = -diff;
            painter.scale(1/xs, 1/ys);
            xs += diff;
            ys += diff;
            painter.restore();
        }
    }

    gear1_rotation -= (20.0 / 150.0);
    gear2_rotation += (20.0 / 112.0);
    gear3_rotation += (20.0 / 114.0);

    pathRender(&painter, device);
}

void CommonRenderer::setMode(Mode m)
{
    mode = m;
}

enum RenderType
{
    Render,
    OpenGL
};

extern RenderType renderer;


void CommonRenderer::printFrameRate()
{
    QTime ctime = QTime::currentTime();

    int ms = ptime.msecsTo(ctime);
    double fc = (double)(frame_cnt*1000)/ms;
    printf("Ver=1, GL=%d, Result:%d/%d=%.3f\n", ((renderer==OpenGL)?1:0), frame_cnt, ms, fc);
    frame_cnt = 0;
    ptime = ctime;
}

