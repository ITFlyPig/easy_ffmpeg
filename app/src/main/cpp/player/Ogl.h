//
// Created by wangyuelin on 2020/11/11.
//

#ifndef FFMPEGTEST_OGL_H
#define FFMPEGTEST_OGL_H

#include "log.h"

extern "C" {
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
}


class Opengl {
private:
    GLuint program;
    GLuint textureID;
public:
    bool CreateProgram();
    bool PrepareTexture();
    void draw(void *pixels, int width, int height, EGLDisplay dpy, EGLSurface surface);

    bool linkShader();
    GLuint loadShader(const char *code, GLint type);
};

#endif //FFMPEGTEST_OGL_H