//
// Created by wangyuelin on 2020/11/11.
//

#ifndef FFMPEGTEST_PLAYER_H
#define FFMPEGTEST_PLAYER_H

#include <pthread.h>
#include <log.h>
#include "Ogl.h"
#include "Egl.h"
#include <constant.h>
#include <unistd.h>
#include "../utils/StringUtil.h"

#endif //FFMPEGTEST_PLAYER_H

class Player {


private:
    AVFormatContext *c = nullptr;
    //视频流的索引
    int videoIndex = -1;
    //文件路劲

    AVCodecID codecId;//解码器id
    AVCodec *codec = NULL;
    AVCodecContext *codecCxt = NULL;

    pthread_t pthread;
    AVPixelFormat m_DstFmt = AV_PIX_FMT_RGBA;//目标格式
    Opengl *opengl;
    Egl *egl;
    ANativeWindow *aNativeWindow;

public:
    char *path;
    int m_nDstWidth = 0;//目标视频宽度
    int m_nDstHeight = 0;//目标视频高度
    int delay = 0;//每帧睡眠的时间

public:

    void newThread(char *path, ANativeWindow *aNativeWindow);

    int open(char *path);

    int decode();

    int release();

    static void *openVideo(void *player);

    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
};
