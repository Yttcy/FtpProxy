//
// Created by tomatoo on 2/9/23.
//

#include <sys/time.h>

#include "Time.h"
#include "Log.h"
#include "PublicParameters.h"
#include "Epoll.h"


TimeNode::TimeNode(std::shared_ptr<Epoll> &epoll):
epoll_(epoll),
expiredtime_(0)
{

}

TimeNode::~TimeNode(){
    PROXY_LOG_INFO("destroy time node");
}

void TimeNode::Start(TimeoutFunc &&func, int timeout) {
    func_ = func;

    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC,&ts);
    expiredtime_ = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000) + timeout;
    auto epoll = epoll_.lock();
    if(epoll != nullptr){
        auto thisPtr = shared_from_this();
        iter_ = epoll->AddTimer(thisPtr);
    }
}

void TimeNode::Stop(){
    std::move(func_);
    func_ = nullptr;
    Update(0);
}

//更新之后要在TimeManager中更新以下，有可能不同定时事件超时时间是相同的
void TimeNode::Update(int timeout) {
    auto timeManager = timeManager_.lock();
    timeManager->UpdateTimeNode(shared_from_this(),timeout);
}

bool TimeNode::IsTimeout() const{
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC,&ts);
    long long temp = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
    PROXY_LOG_DEBUG("time now compare to time out: %lld %lld",temp,expiredtime_);
    if(temp < expiredtime_){
        return false;
    }else{
        return true;
    }
}

void TimeNode::HandleTimeout(){
    func_();
}

void TimeNode::SetTimeManager(std::shared_ptr<TimeManager> &timer) {
    timeManager_ = timer;
}

long long TimeNode::GetExpTime()const {
    return expiredtime_;
}

auto TimeNode::GetIter() {
    return iter_;
}

void TimeNode::SetExpTime(long long expTime) {
    expiredtime_ = expTime;
}

void TimeNode::SetIter(std::set<std::shared_ptr<TimeNode>>::iterator &iter) {
    iter_ = iter;
}

TimeManager::TimeManager() = default;


NodeIter TimeManager::AddTimeNode(const std::shared_ptr<TimeNode>& node){
    return timeQueue_.insert(node).first;
}

void TimeManager::UpdateTimeNode(const std::shared_ptr<TimeNode>& timeNode,int timeout) {

    if(timeout > 0){
        timeQueue_.erase(timeNode->GetIter());
        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC,&ts);
        long long expiredtime = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000) + timeout;
        timeNode->SetExpTime(expiredtime);
        auto iter = timeQueue_.insert(timeNode);
        timeNode->SetIter(iter.first);
    }else{
        timeQueue_.erase(timeNode->GetIter());
    }

}

long long TimeManager::GetMinExpTime(){
    long long ret = EPOLL_TIMEOUT;

    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC,&ts);
    long long timenow = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);

    if(timeQueue_.begin() != timeQueue_.end()){
        auto minNode = *(timeQueue_.begin());
        ret = minNode->GetExpTime();
    }else{
        //没有定时任务，直接返回
        return ret;
    }
    //万一 timenow 大于 ret 呢
    return ret > timenow ? ret - timenow : 10;
}

void TimeManager::HandleExpiredEvents(){
    for(auto iter = timeQueue_.begin();iter != timeQueue_.end();++iter){
        if((*iter)->IsTimeout()){
            auto temp = iter;
            ++iter;
            (*temp)->HandleTimeout();
        }else{
            break;
        }
    }
}