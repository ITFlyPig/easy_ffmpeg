//
// Created by wangyuelin on 2020/11/19.
//

#ifndef FFMPEGTEST_ENCODER_H
#define FFMPEGTEST_ENCODER_H

#include "log.h"
#include "../utils/StringUtil.h"

#define STREAM_FRAME_RATE 25 //视频的帧率
#define STREAM_PIX_FMT  AV_PIX_FMT_YUV420P  //输出视频的格式

class Encoder {

private:
    //输出视频的路径
    const char * outPath;
    //输出视频的宽度
    int width;
    //输出视频的高度
    int height;

    //格式上下文
    AVFormatContext *oc = NULL;
    //输出格式
    AVOutputFormat *fmt = NULL;
    AVCodec *video_codec = NULL;
    AVDictionary *opt = NULL;
    AVStream *st;
public:
    AVCodecContext *enc;

public:
    int open();
    int encode(AVFrame *frame);
    void close();

    Encoder(const char *outPath, int width, int height);
};

#endif //FFMPEGTEST_ENCODER_H
