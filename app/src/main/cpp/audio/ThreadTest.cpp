//
// Created by wangyuelin on 2020/11/21.
//
#include "ThreadTest.h"

void ThreadTest::startThread() {
//    th = std::thread(std::bind(&ThreadTest::doSomeThing, this));
    th = std::thread(&ThreadTest::doSomeThing, this);
//    th.join();
}

void ThreadTest::doSomeThing() {
    sleep(3);

}
