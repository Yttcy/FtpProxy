//
// Created by tomatoo on 2/9/23.
//

#include <sys/time.h>

#include "Time.h"
#include "Log.h"
#include "PublicParameters.h"

TimeNode::TimeNode(int timeout,TimeoutFunc&& func):
expiredtime_(0),
func_(func)
{
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC,&ts);
    expiredtime_ = ((ts.tv_sec  * 1000) + (ts.tv_nsec / 1000000)) + timeout;
}

TimeNode::~TimeNode(){
    PROXY_LOG_INFO("destroy time node");
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

void TimeNode::SetExpTime(long long expTime) {
    expiredtime_ = expTime;
}

TimeManager::TimeManager() = default;


void TimeManager::AddTimeNode(const std::shared_ptr<TimeNode>& node){
    timeQueue_.insert({node->GetExpTime(),node});
}

void TimeManager::UpdateTimeNode(const std::shared_ptr<TimeNode>& timeNode,int timeout) {

    long long oldTime = timeNode->GetExpTime();
    auto iter = timeQueue_.lower_bound(oldTime);

    for(;iter != timeQueue_.end();++iter){
        if(iter->first == timeNode->GetExpTime()){
            if(iter->second == timeNode){
                //删除超时任务,直接返回，不存在迭代器失效问题
                if(timeout == 0){
                    timeQueue_.erase(iter);
                    return;
                }
                //删除了 重新添加
                break;
            }
            //超时时间相同，但是超时事件不同
            continue;
        }else{
            iter = timeQueue_.end();
            break;
        }
    }


    if(iter != timeQueue_.end()){
        timeQueue_.erase(iter);

        struct timespec ts{};
        clock_gettime(CLOCK_MONOTONIC,&ts);
        long long expiredtime = ((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000)) + timeout;

        timeNode->SetExpTime(expiredtime);
        timeQueue_.insert({timeNode->GetExpTime(), timeNode});
    }
}

long long TimeManager::GetMinExpTime() {
    long long ret = EPOLL_TIMEOUT;

    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC,&ts);
    long long timenow = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);

    if(timeQueue_.begin() != timeQueue_.end()){
        ret = timeQueue_.begin()->first;
    }else{
        //没有定时任务，直接返回
        return ret;
    }
    //万一 timenow 大于 ret 呢
    return ret > timenow ? ret - timenow : 10;
}

void TimeManager::HandleExpiredEvents(){
    for(auto iter = timeQueue_.begin();iter != timeQueue_.end();++iter){
        if(iter->second->IsTimeout()){
            iter->second->HandleTimeout();
            auto temp = iter;
            ++iter;
            timeQueue_.erase(temp);
        }else{
            break;
        }
    }
}