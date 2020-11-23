//
// Created by wangyuelin on 2020/11/21.
//
#include "RenderAudio.h"

static int RET_ERROR = -1;
static int RET_SUCCESS = 1;

//音频播放的缓冲区回调
static void play_pcm_callback(SLAndroidSimpleBufferQueueItf bf, void *renderAudio) {
    RenderAudio * render = static_cast<RenderAudio *>(renderAudio);
    render->enqueuePcm();
    //1.获取一帧音频数据
    //2.将获取到的数据放入播放缓冲区
    //SLresult result;//返回结果
    // result = (*iPcmBufferQueue)->Enqueue(iPcmBufferQueue, buffer, 44100 * 2 * 2);
}

int RenderAudio::open() {
    //创建引擎
    SLresult result;//返回结果
    result = slCreateEngine(&engineObj, 0, NULL, 0, NULL, NULL);//第一步创建引擎
    if (result != SL_RESULT_SUCCESS) return RET_ERROR;
    //初始化引擎
    result = (*engineObj)->Realize(engineObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return RET_ERROR;
    //获取引擎接口
    result = (*engineObj)->GetInterface(engineObj, SL_IID_ENGINE,
                                        &iEngine);
    if (result != SL_RESULT_SUCCESS) return RET_ERROR;

    //第二步，创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*iEngine)->CreateOutputMix(iEngine, &mixObj, 1, mids, mreq);
    if (result != SL_RESULT_SUCCESS) return RET_ERROR;
    result = (*mixObj)->Realize(mixObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return RET_ERROR;
//    result = (*mixObj)->GetInterface(mixObj, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
//    if (SL_RESULT_SUCCESS == result) {
//        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings);
//    }

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mixObj};
    SLDataSink audioSink = {&outputMix, NULL};

    //3 配置音频信息
    //缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue que = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    //音频格式
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,////播放pcm格式的数据
            (SLuint32) channels,//    声道数
//            (SLuint32) parameter.sample_rate * 1000,
            static_cast<SLuint32>(OpenSLSampleRate(sampleRate)),//采样率
            SL_PCMSAMPLEFORMAT_FIXED_16,//采样位数
            SL_PCMSAMPLEFORMAT_FIXED_16,
//            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            static_cast<SLuint32>(GetChannelMask(channels)),//立体声或者单声道
            SL_BYTEORDER_LITTLEENDIAN //结束标志
    };
    SLDataSource dataSource = {&que, &pcm};

    //4 创建播放器
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    result = (*iEngine)->CreateAudioPlayer(iEngine, &playerObj, &dataSource, &audioSink,
                                           sizeof(ids) / sizeof(SLInterfaceID), ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "CreateAudioPlayer failed! %d", result);
        return RET_ERROR;
    } else {
        LOGI(TAG, "CreateAudioPlayer success!");
    }
    //初始化播放器
    (*playerObj)->Realize(playerObj, SL_BOOLEAN_FALSE);
    //获取player接口
    result = (*playerObj)->GetInterface(playerObj, SL_IID_PLAY, &iPlayer);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "GetInterface SL_IID_PLAY failed!");
        return RET_ERROR;
    }

    //获取音量接口
    (*playerObj)->GetInterface(playerObj, SL_IID_VOLUME, &iVolume);

    //获取缓冲区接口
    result = (*playerObj)->GetInterface(playerObj, SL_IID_BUFFERQUEUE, &iPcmBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "GetInterface SL_IID_BUFFERQUEUE failed!");
        return RET_ERROR;
    }
    //注册缓冲区回调
    (*iPcmBufferQueue)->RegisterCallback(iPcmBufferQueue, play_pcm_callback, this);

    //设置为播放状态
    (*iPlayer)->SetPlayState(iPlayer, SL_PLAYSTATE_PLAYING);

    //这里通过入队一个空字符串，启动队列开始工作
    (*iPcmBufferQueue)->Enqueue(iPcmBufferQueue, "", 1);

    return RET_SUCCESS;
}


int RenderAudio::close() {
    //停止播放
    if (iPlayer && (*iPlayer)) {
        (*iPlayer)->SetPlayState(iPlayer, SL_PLAYSTATE_STOPPED);
    }
    //清理播放队列
    if (iPcmBufferQueue && (*iPcmBufferQueue)) {
        (*iPcmBufferQueue)->Clear(iPcmBufferQueue);
    }
    //销毁player对象
    if (playerObj && (*playerObj)) {
        (*playerObj)->Destroy(playerObj);
    }
    //销毁混音器
    if (mixObj && (*mixObj)) {
        (*mixObj)->Destroy(mixObj);
    }

    //销毁播放引擎
    if (engineObj && (*engineObj)) {
        (*engineObj)->Destroy(engineObj);
    }

    engineObj = NULL;
    iEngine = NULL;
    mixObj = NULL;
    playerObj = NULL;
    iPlayer = NULL;
    iPcmBufferQueue = NULL;
    iVolume = NULL;
    return RET_SUCCESS;
}

int RenderAudio::OpenSLSampleRate(SLuint32 sampleRate) {
    int samplesPerSec = SL_SAMPLINGRATE_44_1;
    switch (sampleRate) {
        case 8000:
            samplesPerSec = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            samplesPerSec = SL_SAMPLINGRATE_11_025;
            break;
        case 12000:
            samplesPerSec = SL_SAMPLINGRATE_12;
            break;
        case 16000:
            samplesPerSec = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            samplesPerSec = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            samplesPerSec = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            samplesPerSec = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            samplesPerSec = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            samplesPerSec = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            samplesPerSec = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            samplesPerSec = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            samplesPerSec = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            samplesPerSec = SL_SAMPLINGRATE_192;
            break;
        default:
            samplesPerSec = SL_SAMPLINGRATE_44_1;
    }
    return samplesPerSec;
}

int RenderAudio::GetChannelMask(int channels) {
    int channelMask = SL_SPEAKER_FRONT_CENTER;
    switch (channels) {
        case 1:
            channelMask = SL_SPEAKER_FRONT_CENTER;
            break;
        case 2:
            channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
            break;
    }
    return channelMask;
}

void RenderAudio::setPlayVolume(int percent) {
    if (iVolume != NULL) {
        if (percent > 30) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -20);
        } else if (percent > 25) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -22);
        } else if (percent > 20) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -25);
        } else if (percent > 15) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -28);
        } else if (percent > 10) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -30);
        } else if (percent > 5) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -34);
        } else if (percent > 3) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -37);
        } else if (percent > 0) {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -40);
        } else {
            (*iVolume)->SetVolumeLevel(iVolume, (100 - percent) * -100);
        }
    }

}


int RenderAudio::enqueuePcm() {
    if (pcmProvider == nullptr) {
        LOGE(TAG, "pcmProvider 为空,无法提供数据");
        return RET_ERROR;
    }
   PcmInfo *pcmInfo = pcmProvider->provide();
    if (pcmInfo == nullptr) {
        LOGE(TAG, "provide 获取到的数据为空");
        return RET_ERROR;
    }
    SLresult result;
    result = (*iPcmBufferQueue)->Enqueue(iPcmBufferQueue, pcmInfo->data, pcmInfo->size);
    if (result != SL_RESULT_SUCCESS) {
        LOGE(TAG, "enqueuePcm 向opengles 音频队列提供数据失败");
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

RenderAudio::RenderAudio(int channels, long sampleRate, PcmProvider *pcmProvider) : channels(
        channels), sampleRate(sampleRate), pcmProvider(pcmProvider) {}


