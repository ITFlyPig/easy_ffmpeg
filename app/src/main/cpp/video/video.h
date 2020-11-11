//
// Created by wangyuelin on 2020/10/21.
//

#ifndef FFMPEGTEST_VIDEO_H
#define FFMPEGTEST_VIDEO_H

#include <jni.h>
#include <StringUtil.h>
#include <GLRender.h>
#include "constant.h"
#include "log.h"
#include "../egl/egl_core.h"


class Video {

private:

    AVFormatContext *c = nullptr;
    //视频流的索引
    int videoIndex = -1;
    //文件路劲
    char* path;
    AVCodecID codecId;//解码器id
    AVCodec *codec = NULL;
    AVCodecContext * codecCxt = NULL;

    int m_nDstWidth = 0;//目标视频宽度
    int m_nDstHeight = 0;//目标视频高度
    AVPixelFormat m_DstFmt = AV_PIX_FMT_RGBA;//目标格式



public:
    static char *TAG;
    //打开并准备播放
    int Open();
    //开始播放
    int Play(GLRender* glRender, EglCore *eglCore);
    //关闭视频
    int Close();
    Video(char *path, int width, int height, AVPixelFormat dstFmt);

    //申请一帧的空间
    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
    virtual ~Video();


};

#endif //FFMPEGTEST_VIDEO_H
