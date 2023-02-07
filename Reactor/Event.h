//
// Created by tomatoo on 12/27/22.
//

#ifndef FTP_PROXY_EVENT_H
#define FTP_PROXY_EVENT_H

#include <sys/epoll.h>
#include <functional>
#include <optional>
#include <memory>


//继承这个就只能通过create创建了
template<typename T>
class unique_ptr_only{
public:
    typedef std::unique_ptr<T> uniquePtr;

    template<typename... Args>
    static typename unique_ptr_only<T>::uniquePtr create(Args&&... args){
        return std::unique_ptr<T>(new T(args...));
    }

};

typedef std::function<void(int)> Function;
class Event:public unique_ptr_only<Event>{

    friend class unique_ptr_only<Event>;
private:
    explicit Event(int sock);
public:
    int GetSocketFd() const;

    //处理事件
    void Handle(epoll_event& epollEvent);

    //设置可读事件的处理方式
    void SetReadHandle(Function&& function);

private:
    int socketFd_;
    std::optional<Function> readHandle_;
};


#endif //FTP_PROXY_EVENT_H
