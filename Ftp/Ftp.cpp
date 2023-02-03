//
// Created by tomatoo on 2/1/23.
//

#include "EventLoop.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include "Ftp.h"
#include "Util/Log.h"
#include "PublicParameters.h"
#include "MyThread.h"


Ftp::Ftp()
= default;

int Ftp::AddToLoop(std::shared_ptr<EventLoop> &loop) {
    loop_ = loop;
    listenCmdSocket_ = Utils::BindAndListenSocket(PROXY_LISTEN_CMD_PORT);
    PROXY_LOG_INFO("listenCmdSocket is %d",listenCmdSocket_);

    //Ftp代理的事件
    auto event = Event::create(listenCmdSocket_);

    //构造函数中不能调用这个shared_from_this
    auto func = [capture0 = shared_from_this()](int sockfd)->void{ capture0->FtpEvent(sockfd);};
    event->SetReadHandle(func);
    return loop->AddEvent(std::move(event));
}

void Ftp::FtpEvent(int sockfd) {
    assert(sockfd != 0);

    auto client = std::make_shared<Client>();
    client->status_ = STATUS_CONNECTED;
    //同样的
    client->proxyCmdPort_ = PROXY_LISTEN_CMD_PORT;
    client->serverCmdPort_ = SERVER_CMD_PORT;
    client->proxyListenCmdSocket_ = listenCmdSocket_;

    //代理服务器accept客户端到来的控制连接请求，等待认证通过了再去连接服务器，那么就主服务器完成认证吗
    //也就是其它线程不处理USER 和 PASS之外的命令
    struct sockaddr_in clientaddr{};
    socklen_t socklen = sizeof(clientaddr);
    int clientToProxyCmdSocket = Utils::AcceptSocket(listenCmdSocket_, (struct sockaddr *)&clientaddr, &socklen);
    auto ctpCmdEvent = Event::create(clientToProxyCmdSocket);
    ctpCmdEvent->SetReadHandle([capture0 = client->GetClientPtr()](auto && PH1) { capture0->CtpCmdReadCb(std::forward<decltype(PH1)>(PH1)); });
    client->clientToProxyCmdSocket_ = clientToProxyCmdSocket;


    //是代理服务器到ftp服务器的控制连接。当密码通过验证的时候才去连接服务器吗，暂时先不改
    int proxyToServerCmdSocket = Utils::ConnectToServer(ServerIP,SERVER_CMD_PORT);
    auto ptsCmdEvent = Event::create(proxyToServerCmdSocket);
    ptsCmdEvent->SetReadHandle([capture0 = client->GetClientPtr()](auto && PH1){ capture0->PtsCmdReadCb(std::forward<decltype(PH1)>(PH1)); });
    client->proxyToServerCmdSocket_ = proxyToServerCmdSocket;

    //设置IP地址
    client->serverIp_ = ServerIP;

    char ipStr[100] = {};
    inet_ntop(AF_INET,&clientaddr.sin_addr,ipStr,socklen);
    client->clientIp_ = ipStr;
    Utils::GetSockLocalIp(proxyToServerCmdSocket,client->proxyIp_);

    //这里是将Ftp客户端加入到了线程池中去处理，也就是一个客户端只能让一个线程来处理
    auto next_thread = loop_->GetNextThread();
    client->thread_ = next_thread;

    //这个内部的Event没有复制构造函数

    client->epoll_ = next_thread->GetEpoll();
    next_thread->AddAsyncEventHandle(std::move(ctpCmdEvent));
    next_thread->AddAsyncEventHandle(std::move(ptsCmdEvent));

    //task_func中的捕获的参数没有复制构造函数，这个task没法构造

    PROXY_LOG_INFO("new client coming!!!");
}
