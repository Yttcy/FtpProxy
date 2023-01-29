//
// Created by tomatoo on 12/27/22.
//

//
// Created by tomatoo on 8/11/22.
//


#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#define PROXY_LOG_TAG "proxy-print"

#define PROXY_LOG_COLOR_GREE        "\033[1;32m"
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


enum{
    PROXY_LOG_LEVEL_DEBUG,
    PROXY_LOG_LEVEL_INFO,
    PROXY_LOG_LEVEL_WARN,
    PROXY_LOG_LEVEL_ERROR,
    PROXY_LOG_LEVEL_FATAL
};


typedef void (*Proxy_LogFunc)(const std::string& s, const char *file,long line, const char *format, va_list args);


typedef struct Proxy_Logger
{
    std::string p;

    Proxy_LogFunc debug;

    Proxy_LogFunc info;

    Proxy_LogFunc warn;

    Proxy_LogFunc error;

    Proxy_LogFunc fatal;

} Proxy_Logger;

typedef struct Proxy_Logger_Impl{
    Proxy_Logger logger;
    int level{};
}Proxy_Logger_Impl;



static void ProxyLogFnDebug(const std::string& s, const char *file,long line, const char *format, va_list args);
static void ProxyLogFnInfo(const std::string& s, const char *file,long line, const char *format, va_list args);
static void ProxyLogFnWarn(const std::string& s, const char *file, long line, const char *format, va_list args);
static void ProxyLogFnError(const std::string& s, const char *file, long line, const char *format, va_list args);
static void ProxyLogFnFatal(const std::string& s, const char *file, long line, const char *format, va_list args);


static struct Proxy_Logger_Impl s_Proxy_LoggerImpl = {
        {
                PROXY_LOG_TAG,
                ProxyLogFnDebug,
                ProxyLogFnInfo,
                ProxyLogFnWarn,
                ProxyLogFnError,
                ProxyLogFnFatal
        },
        PROXY_LOG_LEVEL_WARN
};


void Proxy_Log(int level, const char *file,long line, const char *format, ...)
{

    if(s_Proxy_LoggerImpl.level > level) return;
    //C语言中可变参数的使用方法，但是这里好像是不需要这个呢
    va_list args;
    va_start(args, format);
    if(level == PROXY_LOG_LEVEL_DEBUG && s_Proxy_LoggerImpl.logger.debug){
        s_Proxy_LoggerImpl.logger.debug(s_Proxy_LoggerImpl.logger.p, file,line, format, args);
    }else if(level == PROXY_LOG_LEVEL_INFO && s_Proxy_LoggerImpl.logger.info){
        s_Proxy_LoggerImpl.logger.info(s_Proxy_LoggerImpl.logger.p, file,line, format, args);
    }else if(level == PROXY_LOG_LEVEL_WARN && s_Proxy_LoggerImpl.logger.warn){
        s_Proxy_LoggerImpl.logger.warn(s_Proxy_LoggerImpl.logger.p, file,line, format, args);
    }else if(level == PROXY_LOG_LEVEL_ERROR && s_Proxy_LoggerImpl.logger.error){
        s_Proxy_LoggerImpl.logger.error(s_Proxy_LoggerImpl.logger.p, file,line, format, args);
    }else if(level == PROXY_LOG_LEVEL_FATAL && s_Proxy_LoggerImpl.logger.fatal){
        s_Proxy_LoggerImpl.logger.fatal(s_Proxy_LoggerImpl.logger.p, file,line, format, args);
    }else{
        printf("No Log Level!\n");
    }

    va_end(args);
}

void ProxyLogCommonFn(const char* p, const char* level, const char* file, long line, const char *format, va_list args, const char* color)
{
    const char* pFileName = strrchr(file, '/') ? (strrchr(file, '/') + 1) : file;
    char msg[1024] = {0};
    vsnprintf(msg, 1024, format, args);
    printf("%s[%s] %s %s(%ld) %s\033[0m\n",  color ? color : "", p ? p : "", level, pFileName, line, msg);
}

static void ProxyLogFnDebug(const std::string& s, const char *file,long line, const char *format, va_list args){
    ProxyLogCommonFn(s.c_str(), YT_LOG_LEVEL_TAG[0], file, line, format, args, nullptr);
}

static void ProxyLogFnInfo(const std::string& s, const char *file,long line, const char *format, va_list args){
    ProxyLogCommonFn(s.c_str(), YT_LOG_LEVEL_TAG[1], file, line, format, args, PROXY_LOG_COLOR_GREE);
}

static void ProxyLogFnWarn(const std::string& s, const char *file,long line, const char *format, va_list args){
    ProxyLogCommonFn(s.c_str(), YT_LOG_LEVEL_TAG[2], file, line, format, args, PROXY_LOG_COLOR_YELLOW);
}

static void ProxyLogFnError(const std::string& s, const char *file,long line, const char *format, va_list args){
    ProxyLogCommonFn(s.c_str(), YT_LOG_LEVEL_TAG[3], file, line, format, args, PROXY_LOG_COLOR_RED);
}

static void ProxyLogFnFatal(const std::string& s, const char *file,long line, const char *format, va_list args){
    ProxyLogCommonFn(s.c_str(), YT_LOG_LEVEL_TAG[4], file, line, format, args, PROXY_LOG_COLOR_PURPLE);
}

__attribute__((unused)) void Proxy_SetLogLevel(int level)
{
    s_Proxy_LoggerImpl.level = level;
}




//write download information to file
//写日志应该也是弄成异步的
int fileFd = open("/home/tomatoo/CLionProjects/FTP_Proxy/DownloadInfo.txt",O_WRONLY | O_APPEND);

void WriteDownloadInfo(std::string& s){
    write(fileFd,s.c_str(),s.size());
}








