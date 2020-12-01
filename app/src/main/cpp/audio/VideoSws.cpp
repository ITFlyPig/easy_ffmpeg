//
// Created by wangyuelin on 2020/12/1.
//
#include "VideoSws.h"
#include "../utils/FrameUtil.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 1;

VideoSws::VideoSws(int srcW, int srcH, AVPixelFormat srcFmt, int dstW, int dstH,
                   AVPixelFormat dstFmt) : srcW(srcW), srcH(srcH), srcFmt(srcFmt), dstW(dstW),
                                           dstH(dstH), dstFmt(dstFmt) {}

int VideoSws::prepare() {
    swsCxt = sws_getContext(srcW, srcH, srcFmt, dstW, dstH, dstFmt, SWS_FAST_BILINEAR, nullptr,
                            nullptr, nullptr);
    if (swsCxt == nullptr) {
        return RET_SUCCESS;
    }
    return RET_ERROR;
}

int VideoSws::end() {
    if (swsCxt != nullptr) {
        sws_freeContext(swsCxt);
    }
    return RET_SUCCESS;
}

int VideoSws::scale(AVFrame *inFrame, AVFrame **outFrame) {
    if (inFrame == nullptr) {
        LOGE(TAG, "传入的Frame为空");
        return RET_ERROR;
    }
    AVFrame *newFrame = FrameUtil::alloc_picture(dstFmt, dstW, dstH);
    if (newFrame == nullptr) {
        LOGE(TAG, "VideoSws 创建新的Frame失败");
        return RET_ERROR;
    }
    newFrame->pts = inFrame->pts;
    int ret = sws_scale(swsCxt, inFrame->data, inFrame->linesize, 0, inFrame->height,
                        newFrame->data, newFrame->linesize);
    if (ret < 0) {
        LOGE(TAG, "VideoSws sws_scale Frame变换失败");
        return RET_ERROR;
    }
    *outFrame = newFrame;
    return RET_SUCCESS;
}
