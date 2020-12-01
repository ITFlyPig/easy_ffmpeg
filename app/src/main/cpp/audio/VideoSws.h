//
// Created by wangyuelin on 2020/12/1.
//

#ifndef FFMPEGTEST_VIDEOSWS_H
#define FFMPEGTEST_VIDEOSWS_H

#include "log.h"
#include "../utils/FrameUtil.h"

class VideoSws {
private:
    int srcW;
    int srcH;
    AVPixelFormat srcFmt;
    int dstW;
    int dstH;
    AVPixelFormat dstFmt;

    SwsContext *swsCxt;

public:
    VideoSws(int srcW, int srcH, AVPixelFormat srcFmt, int dstW, int dstH, AVPixelFormat dstFmt);

    /**
     * 准备变换
     * @return
     */
    int prepare();

    /**
     * 开始变换
     * @param inFrame  需要变换的帧
     * @param outFrame 变换之后的帧
     * @return
     */
    int scale(AVFrame *inFrame, AVFrame **outFrame);

    /**
     * 结束，释放资源
     * @return
     */
    int end();


};

#endif //FFMPEGTEST_VIDEOSWS_H
