//
// Created by wangyuelin on 2020/11/29.
//

#ifndef FFMPEGTEST_VIDEODECODER_H
#define FFMPEGTEST_VIDEODECODER_H

#include <deque>
#include <mutex>
#include "PacketInfo.h"
#include "FrameInfo.h"
#include "log.h"

class MediaPlayer;
class VideoDecoder {

private:

    //最小缓存的包数量
    static const int MIN_PACKET_NUM = 2;
    //存放未解码的包
    std::deque<PacketInfo*> packetQueue;
    //最大缓存的帧数量
    static const int MAX_FRAME_NUM = 4;
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

    MediaPlayer *mediaPlayer;


public:
    VideoDecoder(MediaPlayer *mediaPlayer);

    //将包放到带解压的队列
    void put(PacketInfo* packetInfo);
    //获取解压后的帧数据
    FrameInfo* get();
    //开始解码
    void start();


public:
    //重采样后一帧的大小
    int resampleFrameSize;



};

#endif //FFMPEGTEST_VIDEODECODER_H
