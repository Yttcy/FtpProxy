//
// Created by tomatoo on 1/5/23.
//

#ifndef FTP_PROXY_MYTHREAD_H
#define FTP_PROXY_MYTHREAD_H

#include <memory>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <Event.h>

class Epoll;
class Client;
struct Trans;


class MyThread {
public:

    explicit MyThread();
    void Run();
    void AddAsyncEventHandle(std::unique_ptr<Event> event);
    void AddAsyncTransHandle(Trans&& trans);
    std::shared_ptr<Epoll> GetEpoll();

private:
    static void OnNotify(int sockfd);
    void Notify();
private:
    std::thread th_;
    int ioPipe_[2]{};
    std::shared_ptr<Epoll> epoll_;
};


#endif //FTP_PROXY_MYTHREAD_H
