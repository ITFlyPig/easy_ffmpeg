//
// Created by wangyuelin on 2020/11/27.
//

#ifndef FFMPEGTEST_AUDIODECODER_H
#define FFMPEGTEST_AUDIODECODER_H

#include <deque>
#include <mutex>
#include "PacketInfo.h"
#include "FrameInfo.h"
#include "log.h"
#include "AudioSwr.h"

class MediaPlayer;
class AudioDecoder {

private:

    //最小缓存的包数量
    static const int MIN_PACKET_NUM = 2;
    //存放未解码的包
    std::deque<PacketInfo*> packetQueue;
    //最大缓存的帧数量
    static const int MAX_FRAME_NUM = 10;
    //存放已解码的帧
    std::deque<FrameInfo*> frameQueue;
    std::mutex packeTmtx;
    std::condition_variable putPacketCondition;
    std::condition_variable getPacketCondition;
    //上一次播放的帧
    FrameInfo *lastFrame;

    std::mutex frameTmtx;
    std::condition_variable putFrameCondition;
    std::condition_variable getFrameCondition;

    AVCodecContext *audioCodecCxt = nullptr;
    //音频重采样使用
    AudioSwr *audioSwr = nullptr;
public:
    AudioDecoder(AVCodecContext *audioCodecCxt, AudioSwr *audioSwr);

public:

    //将包放到带解压的队列
    void put(PacketInfo* packetInfo);
    //获取解压后的帧数据
    FrameInfo* get();
    //开始解码
    void start();

public:
    //当前正在播放的帧的pts
    int64_t curPts;
    //当前播放的帧的时间基
    AVRational curTb;
};

#endif //FFMPEGTEST_AUDIODECODER_H
