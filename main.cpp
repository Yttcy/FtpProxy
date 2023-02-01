#include "Log.h"
#include "EventLoop.h"

int main(){
    Proxy_SetLogLevel(PROXY_LOG_LEVEL_INFO);

    std::list<int> queue;
    queue.emplace_back(1);
    queue.emplace_back(2);
    queue.emplace_back(3);
    queue.emplace_back(4);

    //初始化事件环
    EventLoop eventLoop;

    eventLoop.Init();

    eventLoop.Start();
    return 0;
}