//
// Created by tomatoo on 1/5/23.
//

#ifndef FTP_PROXY_MYTHREADPOOL_H
#define FTP_PROXY_MYTHREADPOOL_H

#include <thread>
#define THREAD_NUM 4

class MyThread;


class MyThreadPool {
public:
    explicit MyThreadPool();
    std::shared_ptr<MyThread> GetNextThread();

private:
    long id_;
    long thNum_;
    std::shared_ptr<MyThread> ths_[THREAD_NUM];
};


#endif //FTP_PROXY_MYTHREADPOOL_H
