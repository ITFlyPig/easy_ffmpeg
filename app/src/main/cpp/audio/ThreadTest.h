//
// Created by wangyuelin on 2020/11/21.
//

#ifndef FFMPEGTEST_THREADTEST_H
#define FFMPEGTEST_THREADTEST_H

#include <thread>
#include <unistd.h>
#include "log.h"

class ThreadTest {
public:
    std::thread th;
    void startThread();
    void doSomeThing();

};

#endif //FFMPEGTEST_THREADTEST_H
