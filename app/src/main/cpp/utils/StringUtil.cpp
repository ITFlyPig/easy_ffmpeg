//
// Created by wangyuelin on 2020/10/22.
//

#include <string>
#include "StringUtil.h"

bool StringUtil::isEmpty(const char *s) {
    return s == nullptr || *s == '\0';
}

const char *StringUtil::parseFileName(const char *srcPath) {
    if (isEmpty(srcPath)) {
        return nullptr;
    }
    //转为string操作
    std::string src = srcPath;
    int start = src.find_last_of('/');
    int pointIndex = src.find_last_of('.');
    //简单的检验
    if (start > 0 && start < pointIndex && pointIndex < src.length()) {
        std::string subStr = src.substr(start + 1);
        return subStr.c_str();
    }
    return nullptr;
}

