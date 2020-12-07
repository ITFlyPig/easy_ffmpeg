//
// Created by wangyuelin on 2020/11/27.
//
#include "FrameInfo.h"

FrameInfo::FrameInfo(void *data, long size, int64_t pts) : data(data), size(size), pts(pts) {}

FrameInfo::FrameInfo(AVFrame *videoFrame) : videoFrame(videoFrame) {}

FrameInfo::FrameInfo() {}

FrameInfo::~FrameInfo() {
   /* if (data != nullptr) {
        free(data);
        data = nullptr;
    }
    if (videoFrame != nullptr) {
        av_frame_free(&videoFrame);
        videoFrame = nullptr;
    }*/

}
