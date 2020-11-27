//
// Created by wangyuelin on 2020/11/11.
//

#ifndef FFMPEGTEST_EGL_H
#define FFMPEGTEST_EGL_H

#include "log.h"

extern "C" {
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
}

class Egl {
public:
    EGLDisplay eglDisplay;
    EGLConfig config;
    EGLContext eglContext;
    EGLint eglFormat;
    EGLSurface eglSurface;
public:
    int open(ANativeWindow *nativeWindow);

    int close();

    int render(void *pixel);
};

#endif //FFMPEGTEST_EGL_H