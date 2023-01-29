//
// Created by tomatoo on 12/27/22.
//
#include <cassert>
#include <cerrno>
#include <cstring>

#include "Epoll.h"
#include "Event.h"
#include "Log.h"
#include <Client.h>

#define EPOLL_MAX_FD 1024
#define EPOLL_TIMEOUT 600

Epoll::Epoll():
epollFd_(epoll_create(10)),
eventhandle_(),
transHandle_(),
clients_(std::make_shared<std::unordered_map<int,std::shared_ptr<Client>>>()),
socketNum_(0)
{
    PROXY_LOG_INFO("epollfd is %d",epollFd_);
    assert(epollFd_ != -1);
}

int Epoll::EpollAddEvent(std::shared_ptr<Event>& event) {

    if(socketNum_ >= EPOLL_MAX_FD){
        PROXY_LOG_WARN("over the epoll's max socket num!");
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
        socketMappingToEvent_[socketFd] = event;
    }while(false);

    return ret;
}

int Epoll::EpollDelEvent(const std::shared_ptr<Event>& event) {

    //从epoll中删除描述符
    int ret;
    do{
        int socketFd = event->GetSocketFd();
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
            auto event = socketMappingToEvent_[sock];
            event->Handle(retEvent[i]);
        }


        //这个是处理要加入的客户端
        [&]()->void{

            for(auto tran:transHandle_){
                tran.func(tran.arg);
            }
            transHandle_.clear();

            //处理这个要加一个锁
            std::list<Task> tasks;
            {
                std::lock_guard<std::mutex> lockGuard(lock_);
                tasks = std::move(eventhandle_);
                eventhandle_.clear();
            }
            for(auto task:tasks){
                task.func(task.arg);
            }

        }();

    }while(true);

}

void Epoll::WriteHandle(){

}

void Epoll::AddClient(int ctpSocket,const std::shared_ptr<Client>& client) {
    clients_->insert({ctpSocket,client});
}

void Epoll::DelClient(int ctpSocket){
    clients_->erase(ctpSocket);
}

void Epoll::AddEventHandle(Task&& task){
    eventhandle_.push_back(task);
}

void Epoll::AddTransHandle(Trans &&trans) {
    transHandle_.push_back(trans);
}

void Epoll::EraseClient(int sockfd) {
    clients_->erase(sockfd);
}

std::mutex& Epoll::GetLock() {
    return lock_;
}