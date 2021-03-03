//
// Created by FordChen on 2021/3/1.
//

#ifndef FFMPEG_OPENGL_RENDERING_MYGLWIDGET_H
#define FFMPEG_OPENGL_RENDERING_MYGLWIDGET_H

#include "VideoDecoder.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <memory>

class MyGLWidget: public  QOpenGLWidget, public QOpenGLFunctions_3_3_Core{

    Q_OBJECT
public:
    explicit MyGLWidget(QWidget* parent = nullptr);
    ~MyGLWidget();

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void createDecoder(const std::string& path);
    GLuint make_shader(GLenum type, const char* text);
    GLuint loadTexture(const char *path);
    GLuint loadTexture_memory(unsigned char* data,int width, int height, GLenum format = GL_RGB);
private:
    std::shared_ptr<VideoDecoder> decoder;
    GLuint vertex_shader,fragment_shader,textureImage_location,vbo,textureId,program;
    QOpenGLVertexArrayObject vao;
};


#endif //FFMPEG_OPENGL_RENDERING_MYGLWIDGET_H
