//
// Created by tomatoo on 12/28/22.
//

#include <unistd.h>
#include <cstring>
#include <netinet/in.h>

#include "Client.h"
#include "Util/Utils.h"
#include "Util/Log.h"
#include "Proto.h"
#include <Thread/MyThread.h>
#include <cassert>

Client::Client():
ctpCmdSocket_(-1),
ptsCmdSocket_(-1),
pListenDataSocket_(-1),
ctpDataSocket_(-1),
ptsDataSocket_(-1)
{

};

Client::~Client() {
    PROXY_LOG_INFO("destroy client");
}

std::shared_ptr<Client> Client::GetClientPtr(){
    return shared_from_this();
}

//这是客户端有命令到达
//当客户端关闭的时候，需要将此客户端的定时任务也给关闭掉
void Client::CtpCmdReadCb(int sockfd){

    char buff[BUFFSIZE] = {0};
    //如果读不到内容，那么就关闭套接字，同时从epoll轮询中去掉，clientSocket和ftpServerSocket
    int n = Utils::Readn(sockfd,buff,BUFFSIZE);
    if(n <= 0){
        PROXY_LOG_WARN("client[%d] close the cmd connect!!!",sockfd);
        //socket关闭后，从epoll轮询中去掉客户端控制连接的套接字

        CloseSocket(ctpCmdSocket_);
        CloseSocket(ptsCmdSocket_);
        CloseSocket(pListenDataSocket_);
        CloseSocket(ctpDataSocket_);
        CloseSocket(ptsDataSocket_);

        //关闭定时任务
        timeout_->Stop();

        status_ = STATUS_DISCONNECTED;
        return;
        //这里似乎不需要改变状态了。这个Client对应的事件都没了，那么这个Client对象也没了

        //从客户端读到了内容，那么就处理
    }else{
        //如果读到了数据，那么就回调到上层去处理，暂时就是先回到epoll哪里去处理吧。
        PROXY_LOG_INFO("command received from client : %s",buff);

        //这里需要处理，必须要读到一次完整的命令，才可以做协议解析，反正就默认最大是1024个字节
        cmdBuffer_ += buff;
        int ret = cmdBuffer_.JudgeCmd();
        if(ret < 0){
            return;
        }
        std::string res = cmdBuffer_.GetCompleteCmd();
        PROXY_LOG_FATAL("client complete cmd:%s",res.c_str());

        Data data{};
        memset(&data,0,sizeof(data));
        Utils::SplitCmd(res.data(), data.u.cmd.cmd,data.u.cmd.param);

        //OK,将这个数据交给上层来处理，也就是这个线程对应的Epoll返回的时候来处理这个任务
        auto protoData = std::make_shared<SerializeProtoData>(MAGIC_CLIENT,TYPE_CMD,data);

        Trans trans{[capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyHandleData(std::forward<decltype(PH1)>(PH1)); },protoData};

        this->thread_->AddAsyncTransHandle(std::move(trans));
    }
}

//处理ftp服务器端有命令到达
void Client::PtsCmdReadCb(int sockfd){
    //处理服务器端发给proxy的reply，proxyAcceptClientSocket
    char buff[BUFFSIZE] = {0};

    //这大概就是客户端关闭了连接，发送了QUIT命令之类的
    int n = Utils::Readn(sockfd,buff,BUFFSIZE);
    if(n <= 0){
        PROXY_LOG_WARN("server[%d] close the cmd connect!!!",sockfd);

        CloseSocket(ctpCmdSocket_);
        CloseSocket(ptsCmdSocket_);
        CloseSocket(pListenDataSocket_);
        CloseSocket(ctpDataSocket_);
        CloseSocket(ptsDataSocket_);

        //关闭定时任务
        timeout_->Stop();

        status_ = STATUS_DISCONNECTED;
        return;

    }else{
        PROXY_LOG_INFO("reply received from server : %s",buff);
        //处理ftp服务器的Response,如果有多行，暂时拿第一个状态码作为cmd，后面都是param，包括第一行的-
        //////////

        statusBuffer_ += buff;
        int ret = statusBuffer_.JudgeStatus();
        if(ret < 0){
            return;
        }
        std::string res = statusBuffer_.GetCompleteStatus();

        PROXY_LOG_ERROR("server complete status:%s",res.c_str());

        Data data{};
        memset(&data,0,sizeof(data));
        Utils::SplitStatus(res.data(), data.u.status.status,data.u.status.param);

        //OK,将这个数据交给上层来处理，也就是这个线程对应的Epoll返回的时候来处理这个任务
        auto protoData = std::make_shared<SerializeProtoData>(MAGIC_SERVER,TYPE_STATUS,data);

        Trans trans{[capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyHandleData(std::forward<decltype(PH1)>(PH1)); },protoData};

        this->thread_->AddAsyncTransHandle(std::move(trans));
    }
}


//处理客户端或者ftp服务器连接到来的情况
void Client::ProxyListenDataReadCb(int sockfd) {
    //这是被动模式，要先开一个客户端来连接到这个监听端口，是用来传送数据的

    std::unique_ptr<Event> clientToProxyDataSocketEvent;
    std::unique_ptr<Event> proxyToServerDataSocketEvent;


    if(pasv_mode == 1){ //被动模式
        int clientToProxyDataSocket = Utils::AcceptSocket(sockfd,nullptr,nullptr);        //client <-> proxy
        ctpDataSocket_ = clientToProxyDataSocket;
        clientToProxyDataSocketEvent = Event::create(clientToProxyDataSocket);
        clientToProxyDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->CtpDataReadCb(std::forward<decltype(PH1)>(PH1)); });

        int proxyToServerDataSocket = Utils::ConnectToServer(serverIp_,serverDataPort_);
        ptsDataSocket_ = proxyToServerDataSocket;
        proxyToServerDataSocketEvent = Event::create(proxyToServerDataSocket);
        proxyToServerDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->PtsDataReadCb(std::forward<decltype(PH1)>(PH1)); });
    }
    else if(pasv_mode == 2){    //主动模式,服务器连接过来了
        int proxyToServerDataSocket = Utils::AcceptSocket(pListenDataSocket_,nullptr,nullptr);        //proxy <-> server
        ptsDataSocket_ = proxyToServerDataSocket;
        proxyToServerDataSocketEvent = Event::create(proxyToServerDataSocket);
        proxyToServerDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->PtsDataReadCb(std::forward<decltype(PH1)>(PH1)); });

        struct sockaddr_in cliaddr{};
        socklen_t clilen = sizeof(cliaddr);
        //这就假设port命令发出作为数据连接的IP地址 就是客户端的地址了
        if(getpeername(ctpCmdSocket_,(struct sockaddr *)&cliaddr,&clilen) < 0){
            perror("getpeername() failed: ");
        }

        cliaddr.sin_port = htons(clientDataPort_);
        int clientToProxyDataSocket = Utils::ConnectToServerByAddr(cliaddr); //client <-> proxy
        ctpDataSocket_ = clientToProxyDataSocket;
        clientToProxyDataSocketEvent = Event::create(clientToProxyDataSocket);
        clientToProxyDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->CtpDataReadCb(std::forward<decltype(PH1)>(PH1)); });
    }else{
        PROXY_LOG_FATAL("unknown pasv_mode!!!");
    }

    epoll_->AddEvent(std::move(clientToProxyDataSocketEvent));
    epoll_->AddEvent(std::move(proxyToServerDataSocketEvent));
    PROXY_LOG_INFO("data connecting established");
}

//处理客户端数据到来的情况
void Client::CtpDataReadCb(int sockfd){
    char buff[BUFFSIZE] = {0};
    int n = Utils::Readn(sockfd,buff,BUFFSIZE);
    if(n <= 0){
        CloseSocket(pListenDataSocket_);
        CloseSocket(ctpDataSocket_);
        CloseSocket(ptsDataSocket_);

        if(status_ >= STATUS_PASS_AUTHENTICATED){
            status_ = STATUS_PASS_AUTHENTICATED;
        }
    }else{
        Data data{};
        memset(&data,0,sizeof(data));
        memcpy(data.u.cdata.data,buff,n);
        //OK,将这个数据交给上层来处理，也就是这个线程对应的Epoll返回的时候来处理这个任务
        auto protoData = std::make_shared<SerializeProtoData>(MAGIC_CLIENT,TYPE_DATA,data);

        Trans trans{[capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyHandleData(std::forward<decltype(PH1)>(PH1)); },protoData};

        this->thread_->AddAsyncTransHandle(std::move(trans));
    }
}

void Client::PtsDataReadCb(int sockfd){
    char buff[BUFFSIZE] = {0};
    int n = Utils::Readn(sockfd,buff,BUFFSIZE);
    if(n <= 0){
        PROXY_LOG_WARN("server[%d] close the data socket",ptsDataSocket_);

        CloseSocket(pListenDataSocket_);
        CloseSocket(ctpDataSocket_);
        CloseSocket(ptsDataSocket_);

        if(status_ >= STATUS_PASS_AUTHENTICATED)
            status_ = STATUS_PASS_AUTHENTICATED;
    }
    else{
        Data data{};
        memset(&data,0,sizeof(data));
        memcpy(data.u.cdata.data,buff,n);

        //OK,将这个数据交给上层来处理，也就是这个线程对应的Epoll返回的时候来处理这个任务
        auto protoData = std::make_shared<SerializeProtoData>(MAGIC_SERVER,TYPE_DATA,data);

        Trans trans{[capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyHandleData(std::forward<decltype(PH1)>(PH1)); },protoData};

        this->thread_->AddAsyncTransHandle(std::move(trans));
    }
}

static void DoNothingForCmd(char *buff,char *cmd,char *param){
    sprintf(buff,"%s %s\r\n",cmd,param);
}

//如果是超时，一般是等到下一个客户端命令到来的时候，才回复给客户端421timeout
//如果超时了，直接关闭连接也OK的。
void Client::HandleTimeout() {
    PROXY_LOG_WARN("client[%d] timeout !!!",ctpCmdSocket_);
    //socket关闭后，从epoll轮询中去掉客户端控制连接的套接字
    std::string reply = "421 timeout.\r\n";
    Utils::Writen(ctpCmdSocket_,reply.c_str(),reply.length());
    CloseSocket(ctpCmdSocket_);
    CloseSocket(ptsCmdSocket_);
    CloseSocket(pListenDataSocket_);
    CloseSocket(ctpDataSocket_);
    CloseSocket(ptsDataSocket_);

    timeout_->Stop();

    status_ = STATUS_DISCONNECTED;
}


int Client::ClientCmdHandle(char *cmd,char *param){
    //如果是客户端开启主动模式的命令

    char buff[BUFFSIZE];
    bzero(buff,BUFFSIZE);
    int targetSocket = ptsCmdSocket_;
    std::string reply;
    lastCmd_ = std::move(std::string(cmd));
    if(strcmp(cmd,CMD_USER) == 0){
        targetSocket = ctpCmdSocket_;
        if(status_ >= STATUS_PASS_AUTHENTICATED){
            reply = "530 Can't change to another user.\r\n";
            Utils::Writen(targetSocket,reply.c_str(),reply.length());
            return 0;
        }
        userName_ = param;
        status_ = STATUS_USER_INPUTTED;
        reply = "331 Please specify the password.\r\n";
        strcpy(buff,reply.c_str());
        Utils::Writen(targetSocket,reply.c_str(),reply.length());
    }else if(strcmp(cmd,CMD_PASS) == 0){
        pass_ = param;
        /* 在这里进行简单认证，就先测试一下********************** */
        if(Authenticated() == 0){
            //OK,如果代理这边认证通过了，那么代理就开始向服务端发起认证
            //代理这边认证通过了之后应该就可以确定资产了，然后代理连接对应的资产，OK
            //等到服务端认证通过了之后，代理才向客户端回复230成功或者530失败
            //行，先发送账号
            int proxyToServerCmdSocket = Utils::ConnectToServer(serverIp_,SERVER_CMD_PORT);
            //如果连接失败了，直接关闭与客户端的连接
            if(proxyToServerCmdSocket < 0){
                close(ctpCmdSocket_);
                return 0;
            }

            auto ptsCmdEvent = Event::create(proxyToServerCmdSocket);
            ptsCmdEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1){ capture0->PtsCmdReadCb(std::forward<decltype(PH1)>(PH1)); });
            ptsCmdSocket_ = proxyToServerCmdSocket;
            Utils::GetSockLocalIp(proxyToServerCmdSocket,proxyIp_);
            thread_->AddAsyncEventHandle(std::move(ptsCmdEvent));

            sprintf(buff,"USER %s\r\n",userName_.c_str());
            targetSocket = ptsCmdSocket_;
            status_ = STATUS_PASS_AUTHENTICATED;
            reply = buff;
            Utils::Writen(targetSocket,reply.c_str(),reply.length());
            return 0;
        }else{
            //如果认证失败了，就直接发给客户端530
            reply = "530 Login incorrect.\r\n";
            targetSocket = ctpCmdSocket_;
            strcpy(buff,reply.c_str());
            Utils::Writen(targetSocket,reply.c_str(),reply.length());
        }
    }else if(strcmp(cmd,CMD_QUIT) == 0){
        //这个QUIT不能简单的直接转发给服务器，还要根据连接进行到哪一步来看。
        //因为代理这边有可能还没有连接服务器呢。。。
        if(status_ >= STATUS_PASS_AUTHENTICATED){
            DoNothingForCmd(buff,cmd,param);
            reply = buff;
            PROXY_LOG_DEBUG("send to server:%s",buff);
            Utils::Writen(targetSocket,reply.c_str(),reply.length());
            return 0;
        }
        reply = "221 Goodbye.\r\n";
        targetSocket = ctpCmdSocket_;
        Utils::Writen(targetSocket,reply.c_str(),reply.length());

        //删除任务和定时任务
        epoll_->DelEvent(ctpCmdSocket_);
        timeout_->Stop();
        close(ctpCmdSocket_);
        return 0;
    }else{
        if(status_ < STATUS_PASS_AUTHENTICATED){
            reply = "530 Please login with USER and PASS.\r\n";
            targetSocket = ctpCmdSocket_;
            Utils::Writen(targetSocket,reply.c_str(),reply.length());
        }
    }

    if(status_ < STATUS_PASS_AUTHENTICATED){
        return 0;
    }


    if(strcmp(cmd,CMD_PORT) == 0){
        lastCmd_ = CMD_PORT;
        pasv_mode = 2;

        if(status_ >= STATUS_PROXYDATA_LISTEN){
            CloseSocket(pListenDataSocket_);
            CloseSocket(ctpDataSocket_);
            CloseSocket(ptsDataSocket_);
            PROXY_LOG_DEBUG("close the socket channel");
        }

        //在这儿应该让proxyListenDataSocket监听任意端口,也就是等待服务器通过端口号20连接过来
        int proxyListenDataSocket = Utils::BindAndListenSocket(0); //开启proxyListenDataSocket、bind（）、listen操作
        status_ = STATUS_PROXYDATA_LISTEN;
        proxyDataPort_ = Utils::GetSockLocalPort(proxyListenDataSocket); //这个端口是代理服务器随机生成的
        pListenDataSocket_ = proxyListenDataSocket;
        auto proxyListenDataSocketEvent = Event::create(proxyListenDataSocket);
        proxyListenDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyListenDataReadCb(std::forward<decltype(PH1)>(PH1)); });
        epoll_->AddEvent(std::move(proxyListenDataSocketEvent));

        char param_temp[BUFFSIZE];
        strcpy(param_temp,param);
        clientDataPort_ = Utils::GetPortFromFtpParam(param_temp);
        bzero(buff,BUFFSIZE);
        sprintf(buff,"PORT %s,%d,%d\r\n",proxyIp_.c_str(),proxyDataPort_ / 256,proxyDataPort_ % 256);
    }else if(strcmp(cmd,CMD_PASV) == 0){
        lastCmd_ = CMD_PASV;
        pasv_mode = 1;
        DoNothingForCmd(buff,cmd,param);
    }else{
        DoNothingForCmd(buff,cmd,param);
    }

    PROXY_LOG_DEBUG("send to server:%s",buff);
    Utils::Writen(targetSocket,buff,strlen(buff));
    return 0;
}


//这里完全根据服务端返回的状态码来判断是有问题的，不同的客户端命令可能返回相同的状态码，所有要结合客户端的命令来判断
//如果结合客户端命令来的话，那么if-else就比较多了。
int Client::ServerStatusHandle(char *status,char *param){

    char buff[BUFFSIZE];
    bzero(buff,BUFFSIZE);
    int targetSocket = ctpCmdSocket_;
    if(lastCmd_ == CMD_PASV){
        if(strcmp(status,"227") == 0){
            pasv_mode = 1;

            if(status_ >= STATUS_PROXYDATA_LISTEN){
                CloseSocket(pListenDataSocket_);
                CloseSocket(ctpDataSocket_);
                CloseSocket(ptsDataSocket_);
            }

            int proxyListenDataSocket = Utils::BindAndListenSocket(0); //开启proxyListenDataSocket、bind（）、listen操作
            status_ = STATUS_PROXYDATA_LISTEN;
            pListenDataSocket_ = proxyListenDataSocket;
            auto proxyListenDataSocketEvent = Event::create(proxyListenDataSocket);
            proxyListenDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyListenDataReadCb(std::forward<decltype(PH1)>(PH1)); });
            //这是本地随机生成的端口号
            proxyDataPort_ = Utils::GetSockLocalPort(proxyListenDataSocket);

            epoll_->AddEvent(std::move(proxyListenDataSocketEvent));

            serverDataPort_ = Utils::GetPortFromFtpParam(param + 23);
            bzero(param + 24,BUFFSIZE - 24);
            sprintf(param + 24,"%s,%d,%d).\r\n",proxyIp_.c_str(),proxyDataPort_ / 256,proxyDataPort_ % 256);
            sprintf(buff,"%s%s",status,param);
        }
    }else if(lastCmd_ == CMD_PASS){
        //这是代理向服务端发送了账号之后
        if(strcmp(status,"331") == 0){
            targetSocket = ptsCmdSocket_;
            sprintf(buff,"PASS %s\r\n",pass_.c_str());
        }
        if(strcmp(status,"230") == 0){
            targetSocket = ctpCmdSocket_;
            sprintf(buff,"%s%s\r\n",status,param);
        }
        //这个代表代理连接服务器成功了
        if(strcmp(status,"220") == 0){
            return 0;
        }
    }else{
        targetSocket = ctpCmdSocket_;
        sprintf(buff,"%s%s\r\n",status,param);
    }

    assert(targetSocket > 0);
    PROXY_LOG_DEBUG("send to client:%s",buff);
    Utils::Writen(targetSocket,buff,strlen(buff));
    return 0;
}


//现在是什么都不做，就先直接发送就可以了
int Client::ClientDataHandle(char *data) {
    //这个应该在那个联合数据里指定这个data的长度
    Utils::Writen(ptsDataSocket_,data,strlen(data));
    return 0;
}

//这个也是直接发送
int Client::ServerDataHandle(char *data) {
    Utils::Writen(ctpDataSocket_,data,strlen(data));
    return 0;
}

void Client::ProxyHandleData(const std::shared_ptr<SerializeProtoData>& serializeProtoData) {
    char magic = serializeProtoData->GetMagic();
    char type = serializeProtoData->GetType();
    Data data = serializeProtoData->GetData();

    //收到了数据，更新一下超时时间
    timeout_->Update(CLIENT_TIMEOUT);

    //如果是客户端
    if(magic == MAGIC_CLIENT){
        switch(type){
            case TYPE_CMD:
                PROXY_LOG_FATAL("client cmd[%s],param[%s]",data.u.cmd.cmd,data.u.cmd.param);
                ClientCmdHandle(data.u.cmd.cmd,data.u.cmd.param);
                break;
            case TYPE_DATA:
                PROXY_LOG_FATAL("client data[%s]",data.u.cdata.data);
                ClientDataHandle(data.u.cdata.data);
                break;
            default:
                PROXY_LOG_FATAL("unknown type");
        }
    }else if(magic == MAGIC_SERVER){
        switch(type){
            case TYPE_STATUS:
                PROXY_LOG_ERROR("server status[%s],param[%s]",data.u.status.status,data.u.status.param);
                ServerStatusHandle(data.u.status.status,data.u.status.param);
                break;
            case TYPE_DATA:
                PROXY_LOG_ERROR("server data[%s]",data.u.sdata.data);
                ServerDataHandle(data.u.sdata.data);
                break;
            default:
                PROXY_LOG_FATAL("unknown type");
        }
    }else{
        PROXY_LOG_FATAL("unknown magic");
    }
}

int Client::Authenticated() {
    auto position = userName_.find('@');
    //没找到就先使用默认的IP地址
    if(position == std::string::npos){
        serverIp_ = "127.0.0.1";
        if(userName_ != "ftpuser" || pass_ != "qasa55567"){
            return -1;
        }
        return 0;
    }
    std::string ip(userName_.begin()+position+1,userName_.end());
    std::string realName(userName_.begin(),userName_.begin()+position);
    userName_ = std::move(realName);
    serverIp_ = std::move(ip);

    if(userName_ != "ftpuser" || pass_ != "qasa55567"){
        return -1;
    }
    //这里验证一下IP合不合法？
    return 0;
}

int Client::CloseSocket(int &sockfd) {
    if(sockfd < 0){
        return 0;
    }
    int ret = epoll_->DelEvent(sockfd);
    close(sockfd);
    sockfd = -1;
    return ret;
}