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
#include <set>

#include "Utils.h"
class Epoll;
class Client;
class TimeManager;

typedef std::function<void(void)> TimeoutFunc;
class TimeNode:public shared_ptr_only<TimeNode>,
public std::enable_shared_from_this<TimeNode>{
    friend class shared_ptr_only<TimeNode>;
    explicit TimeNode(std::shared_ptr<Epoll>& epoll);
public:
    virtual ~TimeNode();
    void Start(TimeoutFunc&& func,int timeout);
    void Stop();
    void Update(int timeout); //如果timeout为0，删除定时任务
    bool IsTimeout() const;
    long long GetExpTime() const;
    auto GetIter();
    void SetExpTime(long long expTime);
    void SetIter(std::set<std::shared_ptr<TimeNode>>::iterator& iter);
    void HandleTimeout();
    void SetTimeManager(std::shared_ptr<TimeManager>& timer);

private:
    long long expiredtime_; //ms为单位
    TimeoutFunc func_;
    std::weak_ptr<TimeManager> timeManager_; //防止循环引用
    std::weak_ptr<Epoll> epoll_;
    std::set<std::shared_ptr<TimeNode>>::iterator iter_; //迭代器，在timeManager中的位置
};

typedef std::set<std::shared_ptr<TimeNode>>::iterator NodeIter;

//set存储定时器
class TimeManager{
    typedef std::shared_ptr<TimeNode> SPTimeNode;
private:
    std::set<std::shared_ptr<TimeNode>> timeQueue_;
public:
    TimeManager();
    virtual ~TimeManager();
    NodeIter AddTimeNode(const SPTimeNode& node);
    void HandleExpiredEvents();
    void UpdateTimeNode(const std::shared_ptr<TimeNode>&,int timeout);
    long long GetMinExpTime();
};


#endif //FTPPROXY_TIME_H
