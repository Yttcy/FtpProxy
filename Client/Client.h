//
// Created by tomatoo on 12/28/22.
//

#ifndef FTP_PROXY_CLIENT_H
#define FTP_PROXY_CLIENT_H

#include <string>
#include <memory>
#include <functional>

#include "Epoll.h"
#include "TcpBuffer.h"

class MyThread;
class SerializeProtoData;
class TcpBuffer;


//将哪些任务分配到其它线程中去呢
typedef std::function<void(int)> Function;
class Client {
public:
    explicit Client();

public:

    void CtpCmdReadCb(int sockfd); //客户端到代理服务器的控制连接回调
    void PtsCmdReadCb(int sockfd); //代理服务器到ftp服务器的控制连接回调
    void ProxyListenDataReadCb(int sockfd); //代理服务器的数据连接监听回调
    void CtpDataReadCb(int sockfd); //客户端到代理服务器数据连接回
    void PtsDataReadCb(int sockfd); //代理服务器到ftp服务器数据连接回调

private:
    void ProxyHandleData(const std::shared_ptr<SerializeProtoData>& serializeProtoData);

    int ClientCmdHandle(char *cmd,char *param);
    int ServerStatusHandle(char *cmd,char *param);
    int ClientDataHandle(char *data);
    int ServerDataHandle(char *data);



public:
    std::string userName_;

    int status_;

    //这个套接字对每个客户端都是唯一的
    int proxyListenCmdSocket_{};

    //下面每个客户端都不是一样的了
    std::shared_ptr<Event> clientToProxyCmdSocketEvent_;
    std::shared_ptr<Event> proxyToServerCmdSocketEvent_;
    std::shared_ptr<Event> proxyListenDataSocketEvent_;
    std::shared_ptr<Event> clientToProxyDataSocketEvent_;
    std::shared_ptr<Event> proxyToServerDataSocketEvent_;

    int lastCmdCount_{};

    //以下端口对每个客户端都一样
    int proxyCmdPort_{};
    int serverCmdPort_{};

    int proxyDataPort_{};
    int serverDataPort_{};
    int clientDataPort_{};

    std::string clientIp_;

    //以下对每个客户端都一样，
    std::string proxyIp_;
    std::string serverIp_;

    TcpBuffer buffer_;

    int pasv_mode{};

    std::shared_ptr<Epoll> epoll_;

    //这个client对应在那个线程中运行
    std::shared_ptr<MyThread> thread_;


};



#endif //FTP_PROXY_CLIENT_H
