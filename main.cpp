#include "Util/Log.h"
#include "EventLoop.h"
#include "Ftp.h"

int main(){
    Proxy_SetLogLevel(PROXY_LOG_LEVEL_INFO);


    //初始化事件环
    auto loop = std::make_shared<EventLoop>();

    auto ftpProxy = std::make_shared<Ftp>();
    ftpProxy->AddToLoop(loop);

    loop->Start();
    return 0;
}