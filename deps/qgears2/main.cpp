#include "qgears.h"
#include "qglgears.h"

#include <QApplication>
#include <QtDebug>

#if defined(Q_WS_X11)
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/extensions/Xrender.h>
#endif

enum RenderType
{
    Render,
    OpenGL
};

RenderType renderer = Render;

int main(int argc, char **argv)
{
    int dark=0;
    QWidget *widget = NULL;
    QApplication app(argc, argv);
    CommonRenderer::Mode mode = CommonRenderer::GEARSFANCY;
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp("-gl", argv[i])) {
            renderer = OpenGL;
        }else
        if (!strcmp("-dark", argv[i])) {
            dark = 1;
        }
        else {
            if (i == (argc-1)) {
                fprintf(stderr, "Usage: %s [-gl]\n", argv[0]);
                exit(1);
            }
        }
    }

    if(renderer==OpenGL){
        widget = new QGLGears();
    } else {
        widget = new QGears();
    }
    if(widget == NULL){
       fprintf(stderr, "OpenGL might be unsupported\n");
       exit(1);
    }
    CommonRenderer *rendererWidget = dynamic_cast<CommonRenderer*>(widget);
    if (rendererWidget) {
        //qDebug()<<"setting mode to "<<mode;
        rendererWidget->setMode(mode);
    }

    QPalette pal = QPalette();
    // set background color - light/dark mode
    if(dark){
        pal.setColor(QPalette::Window, Qt::black);
    }else{
        pal.setColor(QPalette::Window, Qt::white);
    }
    widget->setAutoFillBackground(true);
    widget->setPalette(pal);
    widget->setWindowFlags(Qt::FramelessWindowHint);

    widget->setWindowTitle("hardinfo2 - OpenGL");
    widget->show();
    app.exec();
}
