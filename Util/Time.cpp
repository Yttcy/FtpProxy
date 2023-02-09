//
// Created by tomatoo on 2/9/23.
//

#include <sys/time.h>

#include <utility>
#include "Time.h"
#include "Log.h"

#include "Client.h"


TimeNode::TimeNode(std::shared_ptr<Client> client, int timeout):
spClient(std::move(client)),
expiredtime_(0)
{
    struct timeval now{};
    gettimeofday(&now, nullptr);
    expiredtime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimeNode::~TimeNode() = default;


void TimeNode::Update(int timeout) {
    struct timeval now{};
    gettimeofday(&now, nullptr);
    expiredtime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimeNode::IsTimeout() const{
    struct timeval now{};
    gettimeofday(&now, nullptr);
    long temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    PROXY_LOG_DEBUG("time now compare to time out: %ld %ld",temp,expiredtime_);
    if(temp < expiredtime_){
        return false;
    }else{
        return true;
    }
}

void TimeNode::HandleTimeout() {
    spClient->HandleTimeout();
}

long TimeNode::GetExpTime()const {
    return expiredtime_;
}



TimeManager::TimeManager() = default;


void TimeManager::AddTimeNode(std::shared_ptr<TimeNode>& node) {
    timeQueue_.push(std::move(node));
}


void TimeManager::HandleExpiredEvents() {


    while(!timeQueue_.empty()){
        auto node = timeQueue_.top();
        if(node->IsTimeout()){
            node->HandleTimeout();
            timeQueue_.pop();
        }else{
            break;
        }
    }
}