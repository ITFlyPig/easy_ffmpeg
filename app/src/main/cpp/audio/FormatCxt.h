//
// Created by wangyuelin on 2020/12/4.
//

#ifndef FFMPEGTEST_FORMATCXT_H
#define FFMPEGTEST_FORMATCXT_H

#include "log.h"
#include "../utils/StringUtil.h"
#include "OnFrameCallBack.h"

class FormatCxt {
public:
    //格式上下文
    AVFormatContext *c = nullptr;
    //=======视频==========
    //解码上下文
    AVCodecContext *videoCodecCxt = nullptr;
    //解码器
    AVCodec *videoCodec = nullptr;
    //流索引
    int videoStreamIndex = -1;


    //=======音频==========
    AVCodecContext *audioCodecCxt = nullptr;
    AVCodec *audioCodec = nullptr;
    int audioStreamIndex = -1;

    //媒体文件的路劲
    const char *path;
private:
    bool isAudioOpen = false;
    bool isVideoOpen = false;
public:
    /**
     * 打开媒体文件并获取相关信息
     * @return
     */
    int open();

    /**
     * 解码
     * @param packet
     * @return
     */
    int decode(AVCodecContext *codecCxt, AVPacket *packet, OnFrameCallBack *onFrameCallBack);

    /**
     * 关闭媒体文件，并释放资源
     * @return
     */
    int close();

    bool getAudioOpen() const { return isAudioOpen;}
    bool getVideoOpen() const { return isVideoOpen;}


    FormatCxt(const char *path);

private:
    int findAndOpenCodec(AVCodecID codecId, AVCodecContext **pCodecCxt,
            AVCodec **pCodec, const AVCodecParameters *par);

};

#endif //FFMPEGTEST_FORMATCXT_H
