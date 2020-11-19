//
// Created by wangyuelin on 2020/10/22.
//

#ifndef FFMPEGTEST_STRINGUTIL_H
#define FFMPEGTEST_STRINGUTIL_H

#include <cstring>

class StringUtil {

public:
    //判断字符串是否为空
    static bool isEmpty(const char * s);
    //解析得到文件的名称
    static const char * parseFileName(const char* srcPath);


};

#endif //FFMPEGTEST_STRINGUTIL_H
