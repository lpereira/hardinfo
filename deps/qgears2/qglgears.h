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

#ifndef QGLGEARS_H
#define QGLGEARS_H

#ifndef QT_NO_OPENGL
#include <QGLWidget>


#include "commonrenderer.h"

class QPaintEvent;
class QPainter;

class QGLGears : public QGLWidget,
                 public CommonRenderer
{
    Q_OBJECT
public:
    QGLGears();

protected:
    virtual void paintEvent(QPaintEvent *e);
};

#endif
#endif
