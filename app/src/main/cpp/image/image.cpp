//
// Created by wangyuelin on 2020/10/22.
//


#include "image.h"


int Image::prase(char* path) {
    string tag = "Image::prase";
    int ret = -1;
    AVFormatContext *c = nullptr;
    if (StringUtil::isEmpty(path)) {
        LOGE(TAG, "%s 解析图片失败：图片路劲为空", tag.c_str());
        return ret;
    }
    /*创建流，读取文件头，流必须使用avformat_close_input关闭*/
    ret = avformat_open_input(&c, path, NULL, NULL);
    if (ret < 0) {
        LOGE(TAG, "%s 文件打开失败：%s",tag.c_str(), av_err2str(ret));
        return ret;
    }
    /*通过读取packets获得流相关的信息，这个方法对于没有header的媒体文件特别有用，读取到的packets会被缓存后续使用。*/
    ret = avformat_find_stream_info(c, NULL);
    if (ret < 0) {
        LOGE(TAG, "%s avformat_find_stream_info 获取流信息失败", tag.c_str());
        goto __end;
    }
    //找到video stream
    for (int i = 0; i < c->nb_streams; ++i) {
        AVCodecParameters *codecpar =  c->streams[i]->codecpar;
        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //找到需要的宽和高信息
            ret = 0;
            this->w = codecpar->width;
            this->h = codecpar->height;
        }
    }
    if (ret < 0) {
        LOGD(TAG, "%s 未找到图片宽高等信息", tag.c_str());
    }

    __end:
    avformat_close_input(&c);
    return ret;
}


int Image::toYuv420p(AVFrame **out) {
    int ret = -1;
    const char* tag = "toYuv420p";
    if (StringUtil::isEmpty(path)) {
        LOGE(TAG, "%s 错误：图片路劲path为空", tag);
        return ret;
    }


    return 0;
}


