//
// Created by wangyuelin on 2020/11/11.
//


#include "Egl.h"

int Egl::render(void *pixel) {
    return 0;
}

int Egl::close() {
    EGLBoolean success = eglDestroySurface(eglDisplay, eglSurface);
    if (!success) {
        LOGE(TAG, "eglDestroySurface failure.");
    }

    success = eglDestroyContext(eglDisplay, eglContext);
    if (!success) {
        LOGE(TAG, "eglDestroySurface failure.");
    }

    success = eglTerminate(eglDisplay);
    if (!success) {
        LOGE(TAG, "eglDestroySurface failure.");
    }
    eglSurface = NULL;
    eglContext = NULL;
    eglDisplay = NULL;
    return 0;
}

int Egl::open(ANativeWindow *nativeWindow) {

    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE(TAG, "eglGetDisplay failure.");
        return -1;
    }
    LOGE(TAG, "eglGetDisplay ok");
    EGLint majorVersion;
    EGLint minorVersion;
    EGLBoolean success = eglInitialize(eglDisplay, &majorVersion,
                                       &minorVersion);
    if (!success) {
        LOGE(TAG, "eglInitialize failure.");
        return -1;
    }
    LOGE(TAG, "eglInitialize ok");

    GLint numConfigs;

    //3 获取配置并创建surface
    EGLint configSpec[] = {//可以理解为窗口的配置
            EGL_RED_SIZE, 8, // 8bit R
            EGL_GREEN_SIZE, 8,// 8bit G
            EGL_BLUE_SIZE, 8,// 8bit B
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };

    success = eglChooseConfig(eglDisplay, configSpec, &config, 1,
                              &numConfigs);
    if (!success) {
        LOGE(TAG, "eglChooseConfig failure.");
        return -1;
    }
    LOGE(TAG, "eglChooseConfig ok");

    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT,
                                  attribs);
    if (eglContext == EGL_NO_CONTEXT) {
        LOGE(TAG, "eglCreateContext failure, error is %d", eglGetError());
        return -1;
    }
    LOGE(TAG, "eglCreateContext ok");

    success = eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID,
                                 &eglFormat);
    if (!success) {
        LOGE(TAG, "eglGetConfigAttrib failure.");
        return -1;
    }
    LOGE(TAG, "eglGetConfigAttrib ok");

    eglSurface = eglCreateWindowSurface(eglDisplay, config, nativeWindow, 0);
    if (NULL == eglSurface) {
        LOGE(TAG, "eglCreateWindowSurface failure.");
        return -1;
    }
    LOGE(TAG, "eglCreateWindowSurface ok");

    if (EGL_TRUE != eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        LOGE(TAG, "eglMakeCurrent failed!");
        return false;
    }
    LOGE(TAG, "eglMakeCurrent success!");
    return 0;
}
