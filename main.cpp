#include "Log.h"
#include "EventLoop.h"
#include <algorithm>

int main(){
    Proxy_SetLogLevel(PROXY_LOG_LEVEL_INFO);
    EventLoop eventLoop;
    eventLoop.Start();
    return 0;
}