//
// Created by wangyuelin on 2020/11/27.
//

#ifndef FFMPEGTEST_CLOCK_H
#define FFMPEGTEST_CLOCK_H

class Clock {
private:
    double pts;//理论展示时间
    double ptsDrift;//和系统的时间差值
public:
    Clock(double pts, double ptsDrift);
};

#endif //FFMPEGTEST_CLOCK_H
