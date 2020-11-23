//
// Created by wangyuelin on 2020/11/14.
//
//去除视频中的声音



#ifndef FFMPEGTEST_RMAUDIO_H
#define FFMPEGTEST_RMAUDIO_H

#include "log.h"
#include "../utils/StringUtil.h"
#include "../video/encoder.h"

class RmAudio {
public:
    RmAudio(const char *srcPath, const char *outPath);

    virtual ~RmAudio();

private:
    const char * srcPath;
    const  char * outPath;

    //===========输入参数start================
    AVFormatContext *c = nullptr;
    int videoIndex = -1;
    AVCodecID codecId;
    AVCodec *codec;
    AVCodecContext *codecCxt;
    //===========输入参数end================

    //===========输出参数start================
    AVFormatContext *oc = nullptr;
    AVOutputFormat * oFmt = nullptr;
    AVCodec *encoder = nullptr;
    AVCodecContext * encoderCxt = nullptr;
    AVStream *ost = nullptr;

    int m_nDstWidth = 0;
    int m_nDstHeight = 0;
    AVPixelFormat m_DstFmt;
    //===========输出参数end================


public:
    int startRmAudio();
    //测试视频的编码
    void testEncode();
    //翻转实验
    void reverse();

private:
    //打开输入视频文件
    int openInput();
    //打开输出视频文件
    int openOutput();
    //解码输入文件
    int decode(Encoder *encoder);
    //编码之前解码出来的数据
    int encode(AVFrame *decodedFrame);

    void closeInput();

    void closeOutput();




};



#endif //FFMPEGTEST_RMAUDIO_H
