//
// Created by tomatoo on 1/5/23.
//

#include "MyThread.h"
#include <thread>
#include "Event.h"
#include <fcntl.h>
#include "Epoll.h"
#include <Util/Log.h>

MyThread::MyThread():
epoll_(std::make_shared<Epoll>())
{
    pipe2(ioPipe_,O_NONBLOCK);
    auto event = std::make_shared<Event>(ioPipe_[0]);
    event->SetReadHandle([this](auto && PH1) { OnNotify(std::forward<decltype(PH1)>(PH1)); });
    epoll_->EpollAddEvent(event);
    PROXY_LOG_INFO("ioPipe r[%d],w[%d]",ioPipe_[0],ioPipe_[1]);
}

void MyThread::Run() {
    th_ = std::thread(std::bind(&Epoll::Dispatch,epoll_.get()));
}

void MyThread::AddAsyncEventHandle(Task&& task) {
    {
        std::lock_guard<std::mutex> lockGuard(epoll_->GetLock());
        epoll_->AddAsyncEventHandle(std::forward<Task>(task));
    }
    Notify();
}

void MyThread::AddAsyncTransHandle(Trans &&trans) {
    epoll_->AddAsyncTransHandle(std::forward<Trans>(trans));
}

void MyThread::Notify() {
    char c;
    write(ioPipe_[1],&c,1);
}


void MyThread::OnNotify(int sockfd){
    char buf[1];
    read(sockfd,buf,1);
    PROXY_LOG_INFO("notify the thread!");
}

std::shared_ptr<Epoll> MyThread::GetEpoll() {
    return epoll_;
}