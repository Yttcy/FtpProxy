//
// Created by tomatoo on 12/27/22.
//

#include <memory>
#include "EventLoop.h"
#include "Util/Utils.h"
#include "Client.h"
#include "Util/Log.h"
#include <Thread/MyThreadPool.h>
#include <Thread/MyThread.h>

EventLoop::EventLoop():
epoll_(std::make_shared<Epoll>()),
threadPool_(std::make_shared<MyThreadPool>())
{

}

void EventLoop::Start(){

    epoll_->Dispatch();
    //这里是事件环结束时需要进行的一些处理，暂时没有
}

int EventLoop::AddEvent(std::unique_ptr<Event> event) {
    return epoll_->AddEvent(std::move(event));
}

std::shared_ptr<MyThread> EventLoop::GetNextThread(){
    return threadPool_->GetNextThread();
}
