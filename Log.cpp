//
// Created by tomatoo on 8/11/22.
//


#include <cstdio>
#include <cstdarg>
#include <cstring>


#include "Log.h"

#define PROXY_LOG_TAG "proxy-print"

#define PROXY_LOG_COLOR_GREEN       "\033[1;32m"
#define PROXY_LOG_COLOR_YELLOW      "\033[1;33m"
#define PROXY_LOG_COLOR_RED         "\033[1;31m"
#define PROXY_LOG_COLOR_PURPLE      "\033[1;35m"

static const char *YT_LOG_LEVEL_TAG[5] = {
        "[debug]",
        "[info ]",
        "[warn ]",
        "[error]",
        "[fatal]"
};

typedef struct Proxy_Logger_Impl{
    int level{};
}Proxy_Logger_Impl;

static struct Proxy_Logger_Impl s_Proxy_LoggerImpl = {
        PROXY_LOG_LEVEL_WARN
};

void ProxyLogCommonFn(const char* p, const char* level, const char* file, long line, const char *format, va_list args, const char* color)
{
    const char* pFileName = strrchr(file, '/') ? (strrchr(file, '/') + 1) : file;
    char msg[1024] = {0};
    vsnprintf(msg, 1024, format, args);
    printf("%s[%s] %s %s(%ld) %s\033[0m\n",  color ? color : "", p ? p : "", level, pFileName, line, msg);
}


void Proxy_Log(int level, const char *file,long line, const char *format, ...)
{

    if(s_Proxy_LoggerImpl.level > level) return;
    va_list args;
    va_start(args, format);
    if(level == PROXY_LOG_LEVEL_DEBUG){
        ProxyLogCommonFn(PROXY_LOG_TAG, YT_LOG_LEVEL_TAG[1], file, line, format, args, PROXY_LOG_COLOR_GREEN);
    }else if(level == PROXY_LOG_LEVEL_INFO){
        ProxyLogCommonFn(PROXY_LOG_TAG, YT_LOG_LEVEL_TAG[1], file, line, format, args, PROXY_LOG_COLOR_GREEN);
    }else if(level == PROXY_LOG_LEVEL_WARN){
        ProxyLogCommonFn(PROXY_LOG_TAG, YT_LOG_LEVEL_TAG[2], file, line, format, args, PROXY_LOG_COLOR_YELLOW);
    }else if(level == PROXY_LOG_LEVEL_ERROR){
        ProxyLogCommonFn(PROXY_LOG_TAG, YT_LOG_LEVEL_TAG[3], file, line, format, args, PROXY_LOG_COLOR_RED);
    }else if(level == PROXY_LOG_LEVEL_FATAL){
        ProxyLogCommonFn(PROXY_LOG_TAG, YT_LOG_LEVEL_TAG[4], file, line, format, args, PROXY_LOG_COLOR_PURPLE);
    }else{
        printf("No Log Level!\n");
    }

    va_end(args);
}


__attribute__((unused)) void Proxy_SetLogLevel(int level)
{
    s_Proxy_LoggerImpl.level = level;
}








