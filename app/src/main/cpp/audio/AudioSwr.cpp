//
// Created by wangyuelin on 2020/12/4.
//

#include "AudioSwr.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 1;

int AudioSwr::prepare() {
    pSwrContext = swr_alloc();
    if (pSwrContext == nullptr) {
        LOGE(TAG, "prepare申请SwrContext失败");
        return RET_ERROR;
    }
    // 配置输入/输出通道类型
//    av_opt_set_int(pSwrContext, "in_channel_layout", av_get_default_channel_layout(inChannels), 0);
    av_opt_set_int(pSwrContext, "in_channel_layout", av_get_default_channel_layout(inChannels), 0);
    // 这里 AUDIO_DEST_CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO，即 立体声
    av_opt_set_int(pSwrContext, "out_channel_layout", av_get_default_channel_layout(outChannels),
                   0);
    // 配置输入/输出采样率
    av_opt_set_int(pSwrContext, "in_sample_rate", outSampleRate, 0);
    av_opt_set_int(pSwrContext, "out_sample_rate", inSampleRate, 0);
    // 配置输入/输出数据格式
    av_opt_set_sample_fmt(pSwrContext, "in_sample_fmt", inSampleFmt, 0);
    av_opt_set_sample_fmt(pSwrContext, "out_sample_fmt", outSampleFmt, 0);
    //初始化
    if (swr_init(pSwrContext) < 0) {
        LOGE(TAG, "prepare申请swr_init 初始化失败");
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

int AudioSwr::convert(uint8_t **out, int out_count, const uint8_t **in, int in_count) {
    //先计算重采样后buffer大小
    int resampleSize = av_samples_get_buffer_size(
            NULL,
            outChannels,//通道数
            out_count,//采样个数
            outSampleFmt,//采样格式
            1);
    //申请内存
    *out = (uint8_t *) malloc(resampleSize);
    int ret = swr_convert(pSwrContext, out, out_count, in, in_count);
    if (ret < 0) {
        return ret;
    }
    //如果成功，返回重采样的大小
    return resampleSize;
}

int AudioSwr::end() {
    if (pSwrContext != nullptr) {
        swr_free(&pSwrContext);
        pSwrContext = nullptr;
    }
    return RET_SUCCESS;
}

AudioSwr::AudioSwr(int inChannels, int outChannels, int inSampleRate, int outSampleRate,
                   AVSampleFormat inSampleFmt, AVSampleFormat outSampleFmt) : inChannels(
        inChannels), outChannels(outChannels), inSampleRate(inSampleRate), outSampleRate(
        outSampleRate), inSampleFmt(inSampleFmt), outSampleFmt(outSampleFmt) {}
