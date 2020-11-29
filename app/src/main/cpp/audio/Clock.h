//
// Created by wangyuelin on 2020/11/27.
//

#ifndef FFMPEGTEST_CLOCK_H
#define FFMPEGTEST_CLOCK_H

#include "log.h"

class Clock {
private:
    double pts;//理论展示时间
    double ptsDrift;//和系统的时间差值
public:
    void setClock(double pts);

    double getClock();
};

#endif //FFMPEGTEST_CLOCK_H
