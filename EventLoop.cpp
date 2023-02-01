//
// Created by tomatoo on 12/27/22.
//

#include <memory>
#include <unistd.h>
#include <arpa/inet.h>

#include "EventLoop.h"
#include "Utils.h"
#include "Client.h"
#include "Log.h"
#include <Thread/MyThreadPool.h>
#include <Thread/MyThread.h>

#include "PublicParameters.h"

EventLoop::EventLoop():
epoll_(std::make_shared<Epoll>()),
threadPool_(std::make_shared<MyThreadPool>())
{

}

void EventLoop::Start(){

    epoll_->Dispatch();
    //这里是事件环结束时需要进行的一些处理，暂时没有
}

int EventLoop::AddEvent(std::shared_ptr<Event> &event) {
    return epoll_->EpollAddEvent(event);
}

std::shared_ptr<MyThread> EventLoop::GetNextThread(){
    return threadPool_->GetNextThread();
}
