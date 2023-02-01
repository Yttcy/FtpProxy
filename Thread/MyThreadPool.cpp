//
// Created by tomatoo on 1/5/23.
//
#include <functional>

#include "MyThreadPool.h"
#include "MyThread.h"

MyThreadPool::MyThreadPool():
id_(0),
thNum_(THREAD_NUM)
{
    for(int i=0;i<THREAD_NUM;++i){
        //这里创建线程，然后运行
        ths_[i] = std::make_shared<MyThread>();
        ths_[i]->Run();
    }
}

std::shared_ptr<MyThread> MyThreadPool::GetNextThread() {
    int next = id_ % thNum_;
    ++id_;
    return ths_[next];
}