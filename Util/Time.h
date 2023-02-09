//
// Created by tomatoo on 2/9/23.
//

#ifndef FTPPROXY_TIME_H
#define FTPPROXY_TIME_H

#include <memory>
#include <deque>
#include <queue>

#include "Utils.h"
class Client;

class TimeNode:public shared_ptr_only<TimeNode>{

    friend class shared_ptr_only<TimeNode>;
    TimeNode(std::shared_ptr<Client> http,int timeout);
public:
    ~TimeNode();
    void Update(int timeout);
    bool IsTimeout() const;
    long GetExpTime() const;
    void HandleTimeout();

private:
    long expiredtime_; //ms为单位
    std::shared_ptr<Client> spClient;
};

struct TimeCmp{
    bool operator()(std::shared_ptr<TimeNode> &a,std::shared_ptr<TimeNode> &b)const {
        return a->GetExpTime() > b->GetExpTime();
    }
};

//优先队列存储定时器
class TimeManager{
    typedef std::shared_ptr<TimeNode> SPTimeNode;
private:
    //用优先队列来管理定时任务
    std::priority_queue<SPTimeNode,std::vector<SPTimeNode>,TimeCmp> timeQueue_;
public:
    TimeManager();
    void AddTimeNode(SPTimeNode& node);
    void HandleExpiredEvents();
};




#endif //FTPPROXY_TIME_H
