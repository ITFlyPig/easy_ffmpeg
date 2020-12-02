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

int
ThumbnailUtil::getFrame(AVFormatContext *c, AVCodecContext *codecCxt, int64_t pts, int streamIndex,
                        AVFrame **pFrame) {
    //1、遍历数组，如果有命中的返回，如果没有命中的进入步骤2
    //2、调整缓存数据

    return 0;
}

void ThumbnailUtil::adjustCache() {
    for (;;) {
        std::unique_lock<std::mutex> lck(mtx);
        int makeStart = -1;
        int makeEnd = -1;
        int64_t needDecodeMinPts = -1;

        //旧的pts范围
        bool needDecode = true;
        if (cachedFrames[0] != nullptr && cachedFrames[MAX_CACHED_NUM - 1] != nullptr) {
            int64_t oldMinPts = cachedFrames[0]->pts;
            int64_t oldMaxPts = cachedFrames[MAX_CACHED_NUM - 1]->pts;
            //新的调整后的应该有的pts范围
            int64_t newMinPts = curPts - MAX_CACHED_NUM / 2 * ptsStep;
            int64_t newMaxPts = curPts + MAX_CACHED_NUM / 2 * ptsStep;
            //先尽量复用已解码并cache的Frame
            if (oldMinPts == newMinPts && oldMaxPts == newMaxPts) {
                needDecode = false;
            } else if (newMinPts >= oldMinPts && newMinPts <= oldMaxPts) {
                LOGE(TAG, "复用前半部分");
                int splitIndex = -1;
                for (int i = 0; i < MAX_CACHED_NUM; ++i) {
                    AVFrame *curFrame = cachedFrames[i];
                    if (newMinPts == curFrame->pts) {
                        splitIndex = i;
                        break;
                    }
                }
                //开始移动，将cache里面的Frame移动到数组的前面
                if (splitIndex > 0) {
                    int range = splitIndex;
                    //将不需要的Frame内存释放
                    for (int i = 0; i < splitIndex; ++i) {
                        AVFrame *frame = cachedFrames[i];
                        if (frame != nullptr) {
                            av_frame_free(&frame);
                            cachedFrames[i] = nullptr;
                        }
                    }
                    //将需要的移动到前端
                    for (int i = splitIndex; i < MAX_CACHED_NUM; ++i) {
                        cachedFrames[i - range] = cachedFrames[i];
                        cachedFrames[i] = nullptr;
                    }
                    //记录还需要解码的范围
                    makeStart = MAX_CACHED_NUM - range;
                    makeEnd = MAX_CACHED_NUM - 1;
                    needDecodeMinPts = cachedFrames[makeStart - 1]->pts + ptsStep;
                }
            } else if (newMaxPts >= oldMinPts && newMaxPts <= oldMaxPts) {
                LOGE(TAG, "复用后半部分");
                int splitIndex = -1;
                for (int i = 0; i < MAX_CACHED_NUM; ++i) {
                    AVFrame *curFrame = cachedFrames[i];
                    if (newMaxPts == curFrame->pts) {
                        splitIndex = i;
                        break;
                    }
                }
                //开始移动，将cache里面的Frame移动到数组的后面
                if (splitIndex > 0) {
                    int range = MAX_CACHED_NUM - 1 - splitIndex;
                    //将不需要的Frame内存释放
                    for (int i = splitIndex + 1; i < MAX_CACHED_NUM; ++i) {
                        AVFrame *frame = cachedFrames[i];
                        if (frame != nullptr) {
                            av_frame_free(&frame);
                            cachedFrames[i] = nullptr;
                        }
                    }
                    //将需要的移动到后面
                    if (range > 0) {
                        for (int i = splitIndex; i >= 0; i--) {
                            cachedFrames[i + range] = cachedFrames[i];
                            cachedFrames[i] = nullptr;
                        }
                        //记录还需要解码的范围
                        makeStart = 0;
                        makeEnd = makeStart + range - 1;
                        needDecodeMinPts = cachedFrames[makeEnd + 1]->pts - ptsStep * (range);
                    }


                }

            } else {
                LOGE(TAG, "没有复用1");
                //没有缓存能用
                //将之前的全部释放
                for (int i = 0; i < MAX_CACHED_NUM; ++i) {
                    AVFrame *frame = cachedFrames[i];
                    if (frame != nullptr) {
                        av_frame_free(&frame);
                        cachedFrames[i] = nullptr;
                    }
                }
                makeEnd = MAX_CACHED_NUM - 1;
                makeStart = 0;
                needDecodeMinPts = newMinPts;
            }
        } else {
            LOGE(TAG, "没有复用2");
            makeEnd = MAX_CACHED_NUM - 1;
            makeStart = 0;
            needDecodeMinPts = curPts - MAX_CACHED_NUM / 2;
        }

        //需要解码则开始解码
        if (needDecode && makeStart >= 0 && makeEnd >= 0 && makeStart <= makeEnd) {
            AVPacket *pPacket = nullptr;
            int curIndex = makeStart;
            pPacket = av_packet_alloc();
            if (pPacket == nullptr) {
                LOGE(TAG, "fetchFrame AVPacket申请失败");
                return;
            }
            av_init_packet(pPacket);

            AVFrame *pFrame = nullptr;
            pFrame = av_frame_alloc();
            if (pFrame == nullptr) {
                LOGE(TAG, "fetchFrame AVFrame申请失败");
                return;
            }

            int ret = RET_ERROR;
            //刷新
            avcodec_flush_buffers(codecCxt);

            //定位到I帧位置
            int64_t pts = needDecodeMinPts < 0 ? 0 : needDecodeMinPts;
            ret = av_seek_frame(c, streamIndex, pts, AVSEEK_FLAG_BACKWARD);
            if (ret < 0) {
                LOGE(TAG, "Error av_seek_frame:%s", av_err2str(ret))
                return;
            }

            while (av_read_frame(c, pPacket) == 0 && curIndex <= makeEnd) {
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
                            return;
                        }
                        if (pFrame->pts >= needDecodeMinPts) {
                            cachedFrames[curIndex] = pFrame;
                            curIndex++;

                            if (curIndex <= makeEnd) {
                                //重新申请Frame
                                pFrame = av_frame_alloc();
                                if (pFrame == nullptr) {
                                    LOGE(TAG, "fetchFrame AVFrame申请失败");
                                    return;
                                }
                            } else {
                                break;
                            }

                        }
                    }
                    //结束解码
                    if (curIndex > makeEnd) {
                        break;
                    }
                }
            }

            if (pPacket) {
                av_packet_free(&pPacket);
            }

        }
        getCond.notify_all();
        adjustCond.wait(lck);
        lck.unlock();
    }
}

AVFrame *ThumbnailUtil::tryGetFrame(int64_t pts) {
    std::unique_lock<std::mutex> lck(mtx);
    AVFrame *last = nullptr;
    AVFrame *ret = nullptr;
    for (int i = 0; i < MAX_CACHED_NUM - 1; ++i) {
        AVFrame *frame = cachedFrames[i];
        if (frame != nullptr) {
            if (frame->pts == pts) {
                ret = frame;
                break;
            }
            if (frame->pts > pts) {
                if (last != nullptr && last->pts < pts) {
                    ret = frame;
                    break;
                }
            }

        }
        last = frame;
    }
    lck.unlock();
    return ret;
}

int ThumbnailUtil::waitAdjust() {
    std::unique_lock<std::mutex> lck(mtx);
    adjustCond.notify_one();
    getCond.wait(lck);
    lck.unlock();
    return RET_SUCCESS;
}

AVFrame *ThumbnailUtil::getFrame(int64_t pts) {
    AVFrame *ret = tryGetFrame(pts);

    std::unique_lock<std::mutex> lck(mtx);
    this->curPts = pts;
    if (ret == nullptr) {
        //没有找到，调整之后再找
        adjustCond.notify_all();
        getCond.wait(lck);
    } else {
        //已找到，通知线程调整缓存范围
        adjustCond.notify_all();
    }
    lck.unlock();
    //在找一次
    if (ret == nullptr) {
        ret = tryGetFrame(pts);
    }
    return ret;
}


