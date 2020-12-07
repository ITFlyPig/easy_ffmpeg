//
// Created by wangyuelin on 2020/12/4.
//

#ifndef FFMPEGTEST_ONFRAMECALLBACK_H
#define FFMPEGTEST_ONFRAMECALLBACK_H

#include <libavutil/frame.h>

class OnFrameCallBack {
public:
    virtual void onFrame(AVFrame *frame, AVMediaType mediaType);
};

#endif //FFMPEGTEST_ONFRAMECALLBACK_H
