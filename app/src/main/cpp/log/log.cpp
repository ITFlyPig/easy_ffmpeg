//
// Created by wangyuelin on 2020/10/21.
//

#include "log.h"

void FFLog::log_callback_android(void *ptr, int level, const char *fmt, va_list vl) {
    va_list vl2;
    char *line = static_cast<char *>(malloc(128 * sizeof(char)));
    static int print_prefix = 1;
    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, 128, &print_prefix);
    va_end(vl2);
    line[127] = '\0';
    LOGE(TAG, "%s", line);
    free(line);
}
