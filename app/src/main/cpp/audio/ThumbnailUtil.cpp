//
// Created by wangyuelin on 2020/11/30.
//
#include "ThumbnailUtil.h"

static const int RET_ERROR = -1;
static const int RET_SUCCESS = 1;

int ThumbnailUtil::fetchFrame(AVFormatContext *c, AVCodecContext *codecCxt, int64_t pts,
                              int streamIndex, AVFrame **pOutFrame) {
    if (codecCxt == nullptr || c == nullptr) {
        LOGE(TAG, "fetchFrame 参数codecCxt 或 c 为空");
        return RET_ERROR;
    }
    if (pts < 0) {
        LOGE(TAG, "fetchFrame 参数pts小于0");
        return RET_ERROR;
    }
    AVPacket *pPacket = nullptr;
    pPacket = av_packet_alloc();
    if (pPacket == nullptr) {
        LOGE(TAG, "fetchFrame AVPacket申请失败");
        return RET_ERROR;
    }
    av_init_packet(pPacket);

    AVFrame *pFrame = nullptr;
    pFrame = av_frame_alloc();
    if (pFrame == nullptr) {
        LOGE(TAG, "fetchFrame AVFrame申请失败");
        return RET_ERROR;
    }

    int ret = RET_ERROR;
    //定位到I帧位置
    ret = av_seek_frame(c, streamIndex, pts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE(TAG, "Error av_seek_frame:%s", av_err2str(ret))
        return RET_ERROR;
    }

    bool isFind = false;
    while (av_read_frame(c, pPacket) == 0) {
        if (pPacket->stream_index == streamIndex) {
            ret = avcodec_send_packet(codecCxt, pPacket);
            if (ret < 0) {
                LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(codecCxt, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    break;
                }
                if (ret < 0) {
                    LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
                    return RET_ERROR;
                }
                //找到目标Frame
                if (pFrame->pts >= pts) {
                    *pOutFrame = pFrame;
                    isFind = true;
                    break;
                }
            }

            if (isFind) {
                //还原读取指针到开始
                ret = av_seek_frame(c, streamIndex, 0, AVSEEK_FLAG_ANY);
                if (ret < 0) {
                    LOGE(TAG, "Error av_seek_frame 还原到开始位置失败:%s", av_err2str(ret))
                }
                break;
            }
        }
    }
    return RET_SUCCESS;
}
