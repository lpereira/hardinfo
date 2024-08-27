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

#include "qglgears.h"
#include <QTimer>

QGLGears::QGLGears()
    : QGLWidget(QGLFormat(QGL::SampleBuffers), 0)
{
    setFixedSize(1024, 800);
    setMinimumSize(1024, 800);
    setMaximumSize(1024, 800);
    setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
}

void QGLGears::paintEvent(QPaintEvent *)
{

    renderTo(this);
    QTimer::singleShot(0, this, SLOT(repaint()));
}
