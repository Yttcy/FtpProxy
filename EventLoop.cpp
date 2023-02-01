//
// Created by tomatoo on 12/27/22.
//

#include <memory>
#include <unistd.h>
#include <arpa/inet.h>

#include "EventLoop.h"
#include "Utils.h"
#include "Client.h"
#include "Log.h"
#include <MyThreadPool.h>
#include <MyThread.h>

#include "PublicParameters.h"

EventLoop::EventLoop()
{

}

void EventLoop::Init() {
    epoll_ = std::make_shared<Epoll>();
    threadPool_ = std::make_shared<MyThreadPool>();
}

void EventLoop::Start(){
    int listenCmdSocket = Utils::BindAndListenSocket(PROXY_LISTEN_CMD_PORT);
    PROXY_LOG_INFO("listenCmdSocket is %d",listenCmdSocket);

    auto event = std::make_shared<Event>(listenCmdSocket);

    //当有客户端控制连接到来的回调函数
    auto func = [=](int sockfd)->void{
        auto client = std::make_shared<Client>();
        client->status_ = STATUS_CONNECTED;
        //同样的
        client->proxyCmdPort_ = PROXY_LISTEN_CMD_PORT;
        client->serverCmdPort_ = SERVER_CMD_PORT;
        client->proxyListenCmdSocket_ = listenCmdSocket;

        //代理服务器accept客户端到来的控制连接请求，等待认证通过了再去连接服务器，那么就主服务器完成认证吗
        //也就是其它线程不处理USER 和 PASS之外的命令
        struct sockaddr_in clientaddr{};
        socklen_t socklen = sizeof(clientaddr);
        int clientToProxyCmdSocket = Utils::AcceptSocket(listenCmdSocket, (struct sockaddr *)&clientaddr, &socklen);
        auto ctpCmdEvent = std::make_shared<Event>(clientToProxyCmdSocket);
        ctpCmdEvent->SetReadHandle([capture0 = client->GetClientPtr()](auto && PH1) { capture0->CtpCmdReadCb(std::forward<decltype(PH1)>(PH1)); });
        client->clientToProxyCmdSocketEvent_ = ctpCmdEvent;


        //是代理服务器到ftp服务器的控制连接。当密码通过验证的时候才去连接服务器吗，暂时先不改
        int proxyToServerCmdSocket = Utils::ConnectToServer(ServerIP,SERVER_CMD_PORT);
        auto ptsCmdEvent = std::make_shared<Event>(proxyToServerCmdSocket);
        ptsCmdEvent->SetReadHandle([capture0 = client->GetClientPtr()](auto && PH1){ capture0->PtsCmdReadCb(std::forward<decltype(PH1)>(PH1)); });
        client->proxyToServerCmdSocketEvent_ = ptsCmdEvent;

        //设置IP地址
        client->serverIp_ = ServerIP;

        char ipStr[100] = {};
        inet_ntop(AF_INET,&clientaddr.sin_addr,ipStr,socklen);
        client->clientIp_ = ipStr;
        Utils::GetSockLocalIp(proxyToServerCmdSocket,client->proxyIp_);


        //现在这里应该将这些任务分发出去,新的架构是不是应该当认证通过的时候才放到不同线程中去处理
        auto next_thread = threadPool_->GetNext();
        client->thread_ = next_thread;

        auto task_func = [&](const std::shared_ptr<Client>& client1)->void{
            client1->epoll_ = next_thread->GetEpoll();
            client1->epoll_->EpollAddEvent(ctpCmdEvent);
            client1->epoll_->EpollAddEvent(ptsCmdEvent);
        };

        //现在这个AddEvent是线程安全的了
        next_thread->AddEventHandle({task_func, client});
        PROXY_LOG_INFO("new client coming!!!");
    };


    event->SetReadHandle(func);
    epoll_->EpollAddEvent(event);
    epoll_->Dispatch();

    //这里是事件环结束时需要进行的一些处理，暂时没有
}
