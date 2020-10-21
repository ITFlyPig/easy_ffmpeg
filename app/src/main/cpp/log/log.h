//
// Created by wangyuelin on 2020/10/21.
//

#ifndef FFMPEGTEST_LOG_H
#define FFMPEGTEST_LOG_H

#include <cstdlib>
#include <android/log.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
}

// log标签
#define  TAG    "ffmpegtest"
// 定义info信息
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
// 定义debug信息
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
// 定义error信息
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)


class FFLog {
public:
    static void log_callback_android(void *ptr, int level, const char *fmt, va_list vl);
};

#endif //FFMPEGTEST_LOG_H
