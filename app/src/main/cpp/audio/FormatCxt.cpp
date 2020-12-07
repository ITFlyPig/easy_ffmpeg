//
// Created by wangyuelin on 2020/12/4.
//

#include "FormatCxt.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 1;

int FormatCxt::open() {
    int ret = RET_ERROR;
    if (StringUtil::isEmpty(path)) {
        LOGE(TAG, "要打开的媒体文件路劲为空");
        return RET_ERROR;
    }
    ret = avformat_open_input(&c, path, NULL, NULL);
    if (ret) {
        LOGE(TAG, "媒体文件：%s 打开失败:%s", path, av_err2str(ret))
        return RET_ERROR;
    }
    ret = avformat_find_stream_info(c, NULL);
    if (ret < 0) {
        LOGE(TAG, "avformat_find_stream_info 寻找流信息失败：%s", av_err2str(ret));
        return RET_ERROR;
    }
    //找到视频/音频流的索引
    for (int i = 0; i < c->nb_streams; i++) {
        AVMediaType mediaType = c->streams[i]->codecpar->codec_type;
        if (mediaType == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
        } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
        }
    }
    if (videoStreamIndex == -1 && audioStreamIndex == -1) {
        LOGE(TAG, "未找到视频&音频流的索引");
        return RET_ERROR;
    }
    if (videoStreamIndex >= 0) {
        AVCodecParameters *codecpar = c->streams[videoStreamIndex]->codecpar;
        ret = findAndOpenCodec(codecpar->codec_id, &videoCodecCxt, &videoCodec, codecpar);
        if (ret < 0) {
            LOGE(TAG, "打开视频的解码器失败");
        } else {
            isVideoOpen = true;
        }
    }
    if (audioStreamIndex >= 0) {
        AVCodecParameters *codecpar = c->streams[audioStreamIndex]->codecpar;
        ret = findAndOpenCodec(codecpar->codec_id, &audioCodecCxt, &audioCodec, codecpar);
        if (ret < 0) {
            LOGE(TAG, "打开音频的解码器失败");
        } else {
            isAudioOpen = true;
        }
    }
    if (!isAudioOpen && !isVideoOpen) {
        LOGE(TAG, "打开音频和视频的解码器失败");
        return RET_ERROR;
    }
    //打印基本信息
    av_dump_format(c, videoStreamIndex, path, 0);
    LOGD(TAG, "媒体文件：%s 打开成功", path);

    return RET_SUCCESS;
}

int FormatCxt::close() {
    if (c != nullptr) {
        avformat_close_input(&c);
        avformat_free_context(c);
        c = nullptr;
    }
    if (videoCodecCxt != nullptr) {
        avcodec_free_context(&videoCodecCxt);
        videoCodecCxt = nullptr;
    }
    if (audioCodecCxt != nullptr) {
        avcodec_free_context(&audioCodecCxt);
        audioCodecCxt = nullptr;
    }
    LOGD(TAG, "释放资源成功");
    return RET_SUCCESS;
}

int FormatCxt::findAndOpenCodec(AVCodecID codecId, AVCodecContext **pCodecCxt, AVCodec **pCodec,
                                const AVCodecParameters *par) {
    int ret = RET_ERROR;
    *pCodec = avcodec_find_decoder(codecId);
    if (*pCodec == NULL) {
        LOGE(TAG, "avcodec_find_decoder未找到解码器");
        return RET_ERROR;
    }
    //创建解码器上下文
    *pCodecCxt = avcodec_alloc_context3(*pCodec);
    if (*pCodecCxt == NULL) {
        LOGE(TAG, "解码器上下文创建失败");
        return RET_ERROR;
    }

    //将解析得到的解码参数填充到解码上下文
    ret = avcodec_parameters_to_context(*pCodecCxt, par);
    if (ret < 0) {
        LOGE(TAG, "avcodec_parameters_to_context 填充参数失败：%s", av_err2str(ret));
        return RET_ERROR;
    }

    //打开解码器
    ret = avcodec_open2(*pCodecCxt, *pCodec, NULL);
    if (ret < 0) {
        LOGE(TAG, "解码器打开失败：%s", av_err2str(ret));
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

FormatCxt::FormatCxt(const char *path) : path(path) {}

int FormatCxt::decode(AVCodecContext *codecCxt, AVPacket *packet, OnFrameCallBack *onFrameCallBack) {
    if (packet == nullptr || codecCxt == nullptr) {
        LOGE(TAG, "decode 参数AVPacket || AVCodecContext || onFrame  为空");
        return ERROR_CODE;
    }
    int ret = avcodec_send_packet(codecCxt, packet);
    if (ret < 0) {
        LOGE(TAG, "Error sending a packet for decoding:%s", av_err2str(ret))
        return ERROR_CODE;
    }
    while (ret >= 0) {
        AVFrame *pFrame = av_frame_alloc();
        ret = avcodec_receive_frame(codecCxt, pFrame);
        if (ret == AVERROR(EAGAIN)) {
            av_frame_free(&pFrame);
            break;
        }
        if (ret < 0) {
            LOGE(TAG, "Error during decoding:%s", av_err2str(ret));
            av_frame_free(&pFrame);
            break;
        }
        ret = SUCCESS_CODE;
        if (onFrameCallBack != nullptr) {
            onFrameCallBack->onFrame(pFrame, codecCxt->codec_type);
        }

    }

    return ret;
}
