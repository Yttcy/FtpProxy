//
// Created by tomatoo on 12/27/22.
//
#include <cassert>
#include <cerrno>
#include <cstring>

#include "Epoll.h"
#include "Event.h"
#include "Util/Log.h"
#include <Client.h>

#define EPOLL_MAX_FD 1024
#define EPOLL_TIMEOUT 600

Epoll::Epoll():
epollFd_(epoll_create(10)),
eventhandle_(),
transHandle_(),
socketNum_(0)
{
    PROXY_LOG_INFO("epollfd is %d",epollFd_);
    assert(epollFd_ != -1);
}

int Epoll::AddEvent(std::unique_ptr<Event> event) {

    if(socketNum_ >= EPOLL_MAX_FD){
        PROXY_LOG_WARN("over the epoll max socket num!");
        return -1;
    }

    //添加描述符到epoll中去
    int ret;
    do{
        int socketFd = event->GetSocketFd();
        epoll_event epollEvent{};
        epollEvent.data.fd = socketFd;
        epollEvent.events |= EPOLLIN;
        ret = epoll_ctl(epollFd_,EPOLL_CTL_ADD,socketFd,&epollEvent);
        if(ret == -1){
            PROXY_LOG_ERROR("add socket[%d] to epoll failed",socketFd);
            break;
        }
        ++socketNum_;
        socketMappingToEvent_[socketFd] = std::move(event);
        }while(false);

    return ret;
}

int Epoll::DelEvent(int sockfd) {

    //从epoll中删除描述符
    int ret;
    do{
        int socketFd = sockfd;
        epoll_event epollEvent{};
        epollEvent.events |= EPOLLIN;
        ret = epoll_ctl(epollFd_,EPOLL_CTL_DEL,socketFd,&epollEvent);
        if(ret == -1){
            PROXY_LOG_ERROR("del socket[%d] from epoll failed",socketFd);
            break;
        }
        --socketNum_;
        socketMappingToEvent_.erase(socketFd);
    }while(false);

    return ret;
}



void Epoll::Dispatch(){

    do{
        epoll_event retEvent[EPOLL_MAX_FD+1];
        int nReady = epoll_wait(epollFd_,retEvent,EPOLL_MAX_FD+1,EPOLL_TIMEOUT * 1000);

        if(nReady < 0){
            PROXY_LOG_ERROR("epoll_wait error: %s",strerror(errno));
            continue;
        }

        if(nReady == 0){
            PROXY_LOG_INFO("epoll_wait timeout!!!");
            continue;
        }

        for(int i=0;i<nReady;++i){
            int sock = retEvent[i].data.fd;
            if(socketMappingToEvent_[sock] != nullptr){
                socketMappingToEvent_[sock]->Handle(retEvent[i]);
            }
        }

        [&]()->void{

            //处理read调用返回的数据
            for(auto tran:transHandle_){
                tran.func(tran.arg);
            }
            transHandle_.clear();

            //处理从主线程加入的事件
            std::list<std::unique_ptr<Event>> tasks;
            {
                std::lock_guard<std::mutex> lockGuard(lock_);
                tasks = std::move(eventhandle_);
                eventhandle_.clear();
            }
            for(auto iter = tasks.begin();iter != tasks.end();++iter){
                AddEvent(std::move(*iter));
            }

        }();

    }while(true);

}

void Epoll::WriteHandle(){

}

void Epoll::AddAsyncEventHandle(std::unique_ptr<Event> event){
    std::lock_guard<std::mutex> lockGuard(lock_);
    eventhandle_.push_back(std::move(event));
}

void Epoll::AddAsyncTransHandle(Trans &&trans) {
    transHandle_.push_back(trans);
}
