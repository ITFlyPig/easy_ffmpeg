//
// Created by wangyuelin on 2020/10/27.
//

#ifndef FFMPEGTEST_GLRENDER_H
#define FFMPEGTEST_GLRENDER_H

#include "log.h"
#include "constant.h"


extern "C" {
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
}

enum GL_INIT_STATUS {
    WAIT,     //等待初始化
    SUCCESS,  //初始化成功
    ERROR     //初始化失败
};

//封装OpenGL相关的操作
class CGLRender {
private:


    const char *TAG = "CGLRender";
    const GLfloat m_gVertexCoors[8] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f
    };
    //定点着色器代码
    const char *m_pVertexCode = "attribute vec4 aPosition;\n"
                                "attribute vec2 aCoordinate;\n"
                                "varying vec2 vCoordinate;\n"
                                "void main() {\n"
                                "  gl_Position = aPosition;\n"
                                "  vCoordinate = aCoordinate;\n"
                                "}";

    const GLfloat m_gTextureCoors[8] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 0.0f,
            1.0f, 0.0f
    };
    //片远着色器代码
    const char *m_pTextureCode = "precision mediump float;\n"
                                 "uniform sampler2D uTexture;\n"
                                 "varying vec2 vCoordinate;\n"
                                 "void main() {\n"
                                 "vec4 color = texture2D(uTexture, vCoordinate);\n"
                                 "float gray = (color.r + color.g + color.b)/3.0;\n"
                                 "gl_FragColor = vec4(gray, gray, gray, 1.0);\n"
                                 "}";
    //着色器程序中变量的位置
    GLint m_nVertexPosLoc = -1;
    GLint m_nVertexCoordLoc = -1;
    GLint m_nTextureMatrixLoc = -1;


    //初始化状态
    GL_INIT_STATUS m_eInitStatus = WAIT;
    GLuint m_nProgramId = 0;//创建的GL程序的id
    GLuint m_nTextureId = 0;//纹理id
public:
    int Init();

    void Release();
    void checkGlError(const char* op);
    //创建GL程序
    int CreateProgram();
    int CreateAndBindTexture();
    int BindBitmap(void *pixels, int w, int h);

    void Draw();

private:


    //加载着色器
    GLint __LoadShader(GLenum type, const GLchar *pShaderCode);


};

#endif //FFMPEGTEST_GLRENDER_H
