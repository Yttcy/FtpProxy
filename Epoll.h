//
// Created by tomatoo on 12/27/22.
//

#ifndef FTP_PROXY_EPOLL_H
#define FTP_PROXY_EPOLL_H

#include <sys/epoll.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include <list>
#include <mutex>

#include "Event.h"

class Client;
class SerializeProtoData;

typedef std::function<void(std::shared_ptr<Client>)> HandleAddEventFunc;
typedef std::function<void(std::shared_ptr<SerializeProtoData>)> HandleDataTransFunc;

struct Task{
    HandleAddEventFunc func;
    std::shared_ptr<Client> arg;
};

struct Trans{
    HandleDataTransFunc func;
    std::shared_ptr<SerializeProtoData> arg;
};


class Epoll {
public:
    explicit Epoll();

    //向epoll添加套接字
    int EpollAddEvent(std::shared_ptr<Event> event);

    //从epoll中删除套接字
    int EpollDelEvent(const std::shared_ptr<Event>& event);

    //轮询，开始分发任务
    void Dispatch();

    void WriteHandle();

    void AddAsyncEventHandle(Task&& task);

    void AddAsyncTransHandle(Trans&& trans);

    std::mutex& GetLock();

private:
    std::mutex lock_;
    std::list<Task> eventhandle_; //这里估计会有多线程的问题
    std::list<Trans> transHandle_; //这个没有多线程的问题
    int epollFd_; //epoll专用的描述符
    int socketNum_; //epoll中的描述符数量
    std::unordered_map<int,std::shared_ptr<Event>> socketMappingToEvent_; //套接字到事件的映射
};


#endif //FTP_PROXY_EPOLL_H
