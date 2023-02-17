//
// Created by tomatoo on 12/27/22.
//

#ifndef FTP_PROXY_LOG_H
#define FTP_PROXY_LOG_H

#include <string>

enum{
    PROXY_LOG_LEVEL_DEBUG,
    PROXY_LOG_LEVEL_INFO,
    PROXY_LOG_LEVEL_WARN,
    PROXY_LOG_LEVEL_ERROR,
    PROXY_LOG_LEVEL_FATAL,
    PROXY_LOG_LEVEL_NONE
};

void Proxy_SetLogLevel(int level);

void Proxy_Log(int level, const char *file,long line,const char *format,...);


#define PROXY_LOG_DEBUG(f, ...) Proxy_Log(PROXY_LOG_LEVEL_DEBUG, __FILE__, __LINE__, f, ## __VA_ARGS__)

#define PROXY_LOG_INFO(f, ...)  Proxy_Log(PROXY_LOG_LEVEL_INFO, __FILE__,  __LINE__, f, ## __VA_ARGS__)

#define PROXY_LOG_WARN(f, ...)  Proxy_Log(PROXY_LOG_LEVEL_WARN, __FILE__,  __LINE__, f, ## __VA_ARGS__)

#define PROXY_LOG_ERROR(f, ...) Proxy_Log(PROXY_LOG_LEVEL_ERROR, __FILE__, __LINE__, f, ## __VA_ARGS__)

#define PROXY_LOG_FATAL(f, ...) Proxy_Log(PROXY_LOG_LEVEL_FATAL, __FILE__, __LINE__, f, ## __VA_ARGS__)

#endif //FTP_PROXY_LOG_H