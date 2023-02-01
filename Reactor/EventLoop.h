//
// Created by tomatoo on 12/27/22.
//

#ifndef FTP_PROXY_EVENTLOOP_H
#define FTP_PROXY_EVENTLOOP_H

#include <memory>

#include "Util/Utils.h"
#include "Epoll.h"
#include "Client.h"


class MyThreadPool;


class EventLoop {

public:
    explicit EventLoop();

    void Start(); //事件环开始轮询

    int AddEvent(std::shared_ptr<Event>& event);

    std::shared_ptr<MyThread> GetNextThread();

private:

    std::shared_ptr<Epoll> epoll_;
    std::shared_ptr<MyThreadPool> threadPool_;
};


#endif //FTP_PROXY_EVENTLOOP_H
