//
// Created by wangyuelin on 2020/11/27.
//
#include "Clock.h"


void Clock::setClock(double pts) {
    this->pts = pts;
    //获取当前系统时间
    double time = av_gettime_relative() / 1000000.0;
    this->ptsDrift = pts - time;
}

double Clock::getClock() {
    double time = av_gettime_relative() / 1000000.0;
    return this->ptsDrift + time;
}
