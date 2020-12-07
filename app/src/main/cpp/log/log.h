//
// Created by wangyuelin on 2020/10/21.
//

#ifndef FFMPEGTEST_LOG_H
#define FFMPEGTEST_LOG_H

#include <cstdlib>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include <libavutil/opt.h>

#ifdef __cplusplus
}
#endif

#define LOGD(TAG, FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,TAG,FORMAT,##__VA_ARGS__);
#define LOGI(TAG, FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,TAG,FORMAT,##__VA_ARGS__);
#define LOGW(TAG, FORMAT, ...) __android_log_print(ANDROID_LOG_WARN,TAG,FORMAT,##__VA_ARGS__);
#define LOGE(TAG, FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,TAG,FORMAT,##__VA_ARGS__);

const static char *TAG = "FFlog";

static int ERROR_CODE = -1;
static int SUCCESS_CODE = 1;

class FFLog {
private:
public:
    static void log_callback_android(void *ptr, int level, const char *fmt, va_list vl);
};

#endif //FFMPEGTEST_LOG_H
