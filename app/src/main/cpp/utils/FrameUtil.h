//
// Created by wangyuelin on 2020/11/19.
//

#ifndef FFMPEGTEST_FRAMEUTIL_H
#define FFMPEGTEST_FRAMEUTIL_H

#include "log.h"

class FrameUtil {
public:
    //将传入的帧填充yuv420数据
    static void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height);
    //申请一帧
    static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);

};

#endif //FFMPEGTEST_FRAMEUTIL_H
