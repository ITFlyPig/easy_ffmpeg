//
// Created by wangyuelin on 2020/11/23.
//

#ifndef FFMPEGTEST_MEDIAPROVIDER_H
#define FFMPEGTEST_MEDIAPROVIDER_H

#include "PcmProvider.h"
#include <deque>
#include <mutex>

class MediaProvider {
private:
    std::deque<PcmInfo*> pcmQueue;
    std::mutex mtx;
    std::condition_variable produceAudio;
    std::condition_variable consumeAudio;
    static const int MAX_AUDIO_NUM = 10;
public:

    void produce(PcmInfo *pcmInfo);
    PcmInfo* provide();

};

#endif //FFMPEGTEST_MEDIAPROVIDER_H
