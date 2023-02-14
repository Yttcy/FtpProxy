//
// Created by tomatoo on 2/9/23.
//

#ifndef FTPPROXY_TIME_H
#define FTPPROXY_TIME_H

#include <memory>
#include <deque>
#include <queue>
#include <functional>
#include <map>

#include "Utils.h"
class Client;
class TimeManager;

typedef std::function<void(void)> TimeoutFunc;
class TimeNode:public shared_ptr_only<TimeNode>,
public std::enable_shared_from_this<TimeNode>{
    friend class shared_ptr_only<TimeNode>;
    TimeNode(int timeout,TimeoutFunc&& func);
public:
    virtual ~TimeNode();
    void Update(int timeout);
    bool IsTimeout() const;
    long long GetExpTime() const;
    void SetExpTime(long long expTime);
    void HandleTimeout();
    void SetTimeManager(std::shared_ptr<TimeManager>& timer);

private:
    long long expiredtime_; //ms为单位
    TimeoutFunc func_;
    std::weak_ptr<TimeManager> timeManager_; //防止循环引用
};


//优先队列存储定时器
class TimeManager{
    typedef std::shared_ptr<TimeNode> SPTimeNode;
private:
    std::multimap<long long,std::shared_ptr<TimeNode>> timeQueue_;
public:
    TimeManager();
    void AddTimeNode(const SPTimeNode& node);
    void HandleExpiredEvents();
    void UpdateTimeNode(const std::shared_ptr<TimeNode>&,int timeout);
    long long GetMinExpTime();
};


#endif //FTPPROXY_TIME_H
