//
// Created by tomatoo on 12/27/22.
//

#include <cassert>

#include "Event.h"
#include "Util/Log.h"

Event::Event(int sock):
socketFd_(sock)
{

}

int Event::GetSocketFd() const {
    assert(socketFd_ != 0);
    return socketFd_;
}

void Event::Handle(epoll_event& epollEvent){
    int socketFd = epollEvent.data.fd;
    if(epollEvent.events & EPOLLIN){ //可读
        assert(readHandle_.has_value());
        Function readHandle = readHandle_.value();
        readHandle(socketFd);
    }else{
        PROXY_LOG_FATAL("don't has this process function");
    }
}

void Event::SetReadHandle(std::function<void(int)>&& function){
    readHandle_ = function;
}