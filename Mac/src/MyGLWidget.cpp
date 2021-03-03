//
// Created by FordChen on 2021/3/1.
//

#include "MyGLWidget.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

const char* vertex_shader_text =
        "#version 330 core\n"
        "layout(location = 0) in vec2 vPos;\n"
        "layout(location = 1) in vec2 in_texcoord;\n"
        "out vec2 out_texcoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(vec2(vPos.x,-vPos.y), 0.0, 1.0);\n"
        "    out_texcoord = in_texcoord;\n"
        "}\n";

const char* fragment_shader_text =
        "#version 330 core\n"
        "in vec2 out_texcoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D textureImage;\n"
        "void main()\n"
        "{\n"
        "    color = texture(textureImage, out_texcoord);\n"
        //    "    color = vec4(0.0, 1.0, 0.0, 1.0);\n"
        "}\n";

const float vertices[] = {
        1.0f,  1.0f, 1.0f, 1.0f,
        1.0f,  -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
};

MyGLWidget::MyGLWidget(QWidget* parent):
    QOpenGLWidget(parent),
    decoder(nullptr)
{


}

MyGLWidget::~MyGLWidget(){

}

void MyGLWidget::initializeGL() {

//    this->makeCurrent();
    initializeOpenGLFunctions();

    // GLuint vertex_shader,fragment_shader,textureImage_location,vao,vbo,textureId;

    vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_shader_text);
    fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_shader_text);

    int error = glGetError();
    std::cout << "GL error: " << error<<std::endl;

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    GLint program_ok;
    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (program_ok != GL_TRUE) {
        std::cout << "glGetProgramiv error: " << program_ok << std::endl;
    }

    textureImage_location = glGetUniformLocation(program, "textureImage");

    vao.create();
    glGenBuffers(1, &vbo);

    error = glGetError();
    std::cout << "GL error: " << error << std::endl;

//    glUseProgram(program);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, textureId);
//    glUniform1i(textureImage_location, 0);

    vao.bind();
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    vao.release();
    glBindTexture(GL_TEXTURE_2D,0);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
}

void MyGLWidget::resizeGL(int w, int h) {

    // QOpenGLWidget::resizeGL(w,h);
}

void MyGLWidget::paintGL() {

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render
    // ------
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    decoder->decodeNextFrame();
    textureId = loadTexture_memory(decoder->getFrameData(),decoder->getFrameWidth(),decoder->getFrameHeight());

    glUseProgram(program);
    vao.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    update();
}

void MyGLWidget::createDecoder(const std::string &path) {
    decoder = std::make_shared<VideoDecoder>("../male-single-video-face-smile.mp4");
    this->setFixedWidth(decoder->getFrameWidth());
    this->setFixedHeight(decoder->getFrameHeight());
}

GLuint MyGLWidget::make_shader(GLenum type, const char* text) {
    GLuint shader;
    GLint shader_ok;
    GLsizei log_length;
    char info_log[8192];

    shader = glCreateShader(type);
    if (shader != 0) {
        glShaderSource(shader, 1, (const GLchar**)&text, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
        if (shader_ok != GL_TRUE) {
            std::cout << "ERROR: Failed to compile shader: "
            << ((type == GL_FRAGMENT_SHADER) ? "fragment" : "vertex") <<std::endl;
            glGetShaderInfoLog(shader, 8192, &log_length, info_log);
            std::cout << "ERROR: " << info_log << std::endl;
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint MyGLWidget::loadTexture(const char *path){
    if (!path)

        return 0;

    std::string filename = std::string(path);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // stbi_write_png("/Users/fordchen/Desktop/test_tga_out.png", width, height, nrComponents, data, 0);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

GLuint MyGLWidget::loadTexture_memory(unsigned char* data,int width, int height, GLenum format){
    unsigned int textureID;
    if (data)
    {
        glGenTextures(1, &textureID);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cout << "Texture failed to load at memory " << std::endl;
    }

    return textureID;
}
