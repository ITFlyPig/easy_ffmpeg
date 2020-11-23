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
#include <time.h>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
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

    std::thread th;
    AVPixelFormat m_DstFmt = AV_PIX_FMT_RGBA;//目标格式
    Opengl *opengl;
    Egl *egl;

    //锁
    std::mutex mtx;
    //条件变量
    std::condition_variable produce, consume;
    //存放解码后待渲染的Frame
    std::queue<AVFrame*> rgbFrames;
    //最多缓存的帧数
    const int MAX_CACHE_FRAMES = 20;
    bool isEndDecode = false;
    int64_t last = 0;
private:
    int consumeFrame();

     int produceFrame();

public:
    char *path;
    int m_nDstWidth = 0;//目标视频宽度
    int m_nDstHeight = 0;//目标视频高度
    int delay = 0;//每帧睡眠的时间
    ANativeWindow *aNativeWindow;

public:

    void newThread(char *path, ANativeWindow *aNativeWindow);

    int open(char *path);

    int decode();

    int release();

    int reversePlay();
    std::vector<int64_t> reverseDecode();
    //seek到I帧，然后倒叙播放
    int playByIFrame(int64_t iFramePts, int64_t lastPts);


    static void *openVideo(void *player);

    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
};
