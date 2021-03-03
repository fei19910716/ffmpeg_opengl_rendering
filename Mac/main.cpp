//
// Created by FordChen on 2021/2/3.
//

#include "src/MyGLWidget.h"

#include <QApplication>

int main(int argc, char** argv){


    QApplication app(argc,argv);


    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format); // 设置format
    MyGLWidget w; // 创建窗口应该在设置format之后，否则会crash
    // w.setFormat(format); // 不应该使用这个设置format，否则会报Could not create NSOpenGLContext with shared context, falling back to unshared context.

    w.createDecoder("./male-single-video-face-smile.mp4");
    w.show();
    return app.exec();
}