#include <iostream>
#include "Util/Log.h"
#include "EventLoop.h"
#include "Ftp.h"

int main(){

    Proxy_SetLogLevel(PROXY_LOG_LEVEL_DEBUG);
    //初始化事件环
    auto loop = std::make_shared<EventLoop>();

    auto ftpProxy = Ftp::create();
    ftpProxy->AddToLoop(loop);

    loop->Start();
    return 0;
}


