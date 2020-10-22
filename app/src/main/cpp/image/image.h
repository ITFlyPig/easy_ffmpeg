//
// Created by wangyuelin on 2020/10/22.
//

#ifndef FFMPEGTEST_IMAGE_H
#define FFMPEGTEST_IMAGE_H

#include <string>
//#include "../utils/StringUtil.h"
#include "StringUtil.h"
#include "log.h"

using namespace std;

class Image {
public:
    string path;  //图片文件的路劲
    int w;        //图片的宽
    int h;        //图片的高

public:
    //解析传入的图片，得到一些基本信息
    int prase(char *path);

    //将一张图片解码，然后转为Yuv420p格式的Frame
    int toYuv420p(AVFrame *out);

};

#endif //FFMPEGTEST_IMAGE_H
