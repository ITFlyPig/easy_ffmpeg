//
// Created by wangyuelin on 2020/11/23.
//

#include "MediaProvider.h"


void MediaProvider::produce(PcmInfo *pcmInfo) {
    std::unique_lock<std::mutex> lck(mtx);
    while (pcmQueue.size() > MAX_AUDIO_NUM) {
        produceAudio.wait(lck);
    }
    pcmQueue.push_back(pcmInfo);
    consumeAudio.notify_one();
    lck.unlock();
}

PcmInfo *MediaProvider::provide() {
    PcmInfo *pcmInfo = nullptr;
    std::unique_lock<std::mutex> lck(mtx);
    while (pcmQueue.empty()) {
        consumeAudio.wait(lck);
    }
    pcmInfo = pcmQueue.front();
    pcmQueue.pop_front();
    produceAudio.notify_one();
    lck.unlock();
    return pcmInfo;
}
