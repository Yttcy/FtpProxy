//
// Created by tomatoo on 12/28/22.
//

#ifndef FTP_PROXY_CLIENT_H
#define FTP_PROXY_CLIENT_H

#include <string>
#include <memory>
#include <functional>

#include "Epoll.h"
#include "FtpTcpBuffer.h"

class MyThread;
class SerializeProtoData;
class FtpTcpBuffer;


//补个超时，421 Timeout
typedef std::function<void(int)> Function;
class Client :public std::enable_shared_from_this<Client>{
public:
    explicit Client();
    std::shared_ptr<Client> GetClientPtr();
public:

    void CtpCmdReadCb(int sockfd); //客户端到代理服务器的控制连接回调
    void PtsCmdReadCb(int sockfd); //代理服务器到ftp服务器的控制连接回调
    void ProxyListenDataReadCb(int sockfd); //代理服务器的数据连接监听回调
    void CtpDataReadCb(int sockfd); //客户端到代理服务器数据连接回调
    void PtsDataReadCb(int sockfd); //代理服务器到ftp服务器数据连接回调

private:
    void ProxyHandleData(const std::shared_ptr<SerializeProtoData>& serializeProtoData);

    int ClientCmdHandle(char *cmd,char *param);
    int ServerStatusHandle(char *status,char *param);
    int ClientDataHandle(char *data);
    int ServerDataHandle(char *data);

private:
    int CloseSocket(int &sockfd);
public:
    std::string userName_;
    std::string pass_;

    int status_{};
    //这个套接字对每个客户端都是唯一的
    int pListenCmdSocket_{};

    //下面每个客户端都不是一样的了
    int ctpCmdSocket_{};
    int ptsCmdSocket_{};
    int pListenDataSocket_{};
    int ctpDataSocket_{};
    int ptsDataSocket_{};

    std::string lastCmd_;

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

    FtpTcpBuffer cmdBuffer_;
    FtpTcpBuffer statusBuffer_;

    int pasv_mode{};

    std::shared_ptr<Epoll> epoll_;

    //这个client对应在那个线程中运行
    std::shared_ptr<MyThread> thread_;


};
#endif //FTP_PROXY_CLIENT_H
