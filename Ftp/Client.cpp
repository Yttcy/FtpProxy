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

Client::Client() {

}

std::shared_ptr<Client> Client::GetClientPtr(){
    return shared_from_this();
}

//这是客户端有命令到达
void Client::CtpCmdReadCb(int sockfd){

    char buff[BUFFSIZE] = {0};
    //如果读不到内容，那么就关闭套接字，同时从epoll轮询中去掉，clientSocket和ftpServerSocket
    if (read(sockfd, buff, BUFFSIZE) == 0){

        PROXY_LOG_WARN("client[%d] close the cmd connect!!!",sockfd);
        //socket关闭后，从epoll轮询中去掉客户端控制连接的套接字
        epoll_->DelEvent(clientToProxyCmdSocket_);
        epoll_->DelEvent(proxyToServerCmdSocket_);

        close(sockfd);
        close(proxyToServerCmdSocket_);

        //这里还要删除对应的客户端保存的控制连接，也就是那个unordered_map
        //从客户端读到了内容，那么就处理
    }else{

        //在这里模拟TCP分片，第一次来数据的时候就给buff替换成《USER 》，后续再发送账号。

        //如果读到了数据，那么就回调到上层去处理，暂时就是先回到epoll哪里去处理吧。
        PROXY_LOG_INFO("command received from client : %s",buff);

        //这里需要处理，必须要读到一次完整的命令，才可以做协议解析，反正就默认最大是1024个字节
        buffer_ += buff;
        int ret = buffer_.JudgeCmd();
        if(ret != 0){
            return;
        }
        std::string res = buffer_.GetCompleteCmd();
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
    if(read(sockfd,buff,BUFFSIZE) == 0){
        PROXY_LOG_WARN("server[%d] close the cmd connect!!!",sockfd);
        epoll_->DelEvent(clientToProxyCmdSocket_);
        epoll_->DelEvent(proxyToServerCmdSocket_);
        close(sockfd);
        close(clientToProxyCmdSocket_);
        return;
    }

    PROXY_LOG_INFO("reply received from server : %s",buff);
    //处理ftp服务器的Response,如果有多行，暂时拿第一个状态码作为cmd，后面都是param，包括第一行的-
    //////////



    buffer_ += buff;
    int ret = buffer_.JudgeStatus();
    if(ret != 0){
        return;
    }
    std::string res = buffer_.GetCompleteStatus();

    PROXY_LOG_ERROR("server complete status:%s",res.c_str());

    Data data{};
    memset(&data,0,sizeof(data));
    Utils::SplitStatus(res.data(), data.u.status.status,data.u.status.param);

    //OK,将这个数据交给上层来处理，也就是这个线程对应的Epoll返回的时候来处理这个任务
    auto protoData = std::make_shared<SerializeProtoData>(MAGIC_SERVER,TYPE_STATUS,data);

    Trans trans{[capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyHandleData(std::forward<decltype(PH1)>(PH1)); },protoData};

    this->thread_->AddAsyncTransHandle(std::move(trans));
}


//处理客户端或者ftp服务器连接到来的情况
void Client::ProxyListenDataReadCb(int sockfd) {
    //这是被动模式，要先开一个客户端来连接到这个监听端口，是用来传送数据的

    std::unique_ptr<Event> clientToProxyDataSocketEvent;
    std::unique_ptr<Event> proxyToServerDataSocketEvent;

    if(pasv_mode == 1){
        int clientToProxyDataSocket = Utils::AcceptSocket(sockfd,nullptr,nullptr);        //client <-> proxy
        clientToProxyDataSocket_ = clientToProxyDataSocket;
        clientToProxyDataSocketEvent = Event::create(clientToProxyDataSocket);
        clientToProxyDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->CtpDataReadCb(std::forward<decltype(PH1)>(PH1)); });

        int proxyToServerDataSocket = Utils::ConnectToServer(serverIp_,serverDataPort_);
        proxyToServerDataSocket_ = proxyToServerDataSocket;
        proxyToServerDataSocketEvent = Event::create(proxyToServerDataSocket);
        proxyToServerDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->PtsDataReadCb(std::forward<decltype(PH1)>(PH1)); });
    }
    else if(pasv_mode == 2){    //主动模式,服务器连接过来了
        int proxyToServerDataSocket = Utils::AcceptSocket(proxyListenDataSocket_,nullptr,nullptr);        //proxy <-> server
        proxyToServerDataSocket_ = proxyToServerDataSocket;
        proxyToServerDataSocketEvent = Event::create(proxyToServerDataSocket);
        proxyToServerDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->PtsDataReadCb(std::forward<decltype(PH1)>(PH1)); });

        struct sockaddr_in cliaddr{};
        socklen_t clilen = sizeof(cliaddr);
        //这就假设port命令发出作为数据连接的IP地址 就是客户端的地址了
        if(getpeername(clientToProxyCmdSocket_,(struct sockaddr *)&cliaddr,&clilen) < 0){
            perror("getpeername() failed: ");
        }
        cliaddr.sin_port = htons(clientDataPort_);
        int clientToProxyDataSocket = Utils::ConnectToServerByAddr(cliaddr); //client <-> proxy
        clientToProxyDataSocket_ = clientToProxyDataSocket;
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
    long n;
    char buff[BUFFSIZE] = {0};
    if((n = read(sockfd,buff,BUFFSIZE)) == 0){

        epoll_->DelEvent(proxyListenDataSocket_);
        epoll_->DelEvent(clientToProxyDataSocket_);
        epoll_->DelEvent(proxyToServerDataSocket_);

        close(proxyListenDataSocket_);
        close(clientToProxyDataSocket_);
        close(proxyToServerDataSocket_);
    }
    else{
        Data data{};
        memset(&data,0,sizeof(data));
        memcpy(data.u.cdata.data,buff,n);
        //OK,将这个数据交给上层来处理，也就是这个线程对应的Epoll返回的时候来处理这个任务
        auto protoData = std::make_shared<SerializeProtoData>(MAGIC_CLIENT,TYPE_DATA,data);

        Trans trans{[capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyHandleData(std::forward<decltype(PH1)>(PH1)); },protoData};

        this->thread_->AddAsyncTransHandle(std::move(trans));
    }
}

//这里有点问题，服务端断开连接的同时，客户端也会断开连接，这样的话，close没有关系，但是epoll清除就会有问题，暂时先不管
void Client::PtsDataReadCb(int sockfd){
    long n;
    char buff[BUFFSIZE] = {0};
    if((n = read(sockfd,buff,BUFFSIZE)) == 0){

        epoll_->DelEvent(proxyListenDataSocket_);
        epoll_->DelEvent(clientToProxyDataSocket_);
        epoll_->DelEvent(proxyToServerDataSocket_);
        close(proxyListenDataSocket_);
        close(clientToProxyDataSocket_);
        close(proxyToServerDataSocket_);
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


int Client::ClientCmdHandle(char *cmd,char *param){
    //如果是客户端开启主动模式的命令
    char buff[BUFFSIZE];
    bzero(buff,BUFFSIZE);
    if(strcmp(cmd,CMD_USER) == 0){
        lastCmdCount_ = CMD_USER_COUNT;
        userName_ = param;
        status_ = STATUS_USER_INPUTTED;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_PASS) == 0){
        lastCmdCount_ = CMD_PASS_COUNT;
        /* 在这里进行认证 */
        status_ = STATUS_PASS_AUTHENTICATED;
        /* 连接到服务器 */

        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_ACCT) == 0){
        lastCmdCount_ = CMD_ACCT_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_CWD) == 0){
        lastCmdCount_ = CMD_CWD_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_CDUP) == 0){
        lastCmdCount_ = CMD_CDUP_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_SMNT) == 0){
        lastCmdCount_ = CMD_SMNT_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_QUIT) == 0){
        lastCmdCount_ = CMD_QUIT_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_REIN) == 0){
        lastCmdCount_ = CMD_REIN_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_PORT) == 0){

        lastCmdCount_ = CMD_PORT_COUNT;
        //在这儿应该让proxyListenDataSocket监听任意端口,也就是等待服务器通过端口号20连接过来
        int proxyListenDataSocket = Utils::BindAndListenSocket(0); //开启proxyListenDataSocket、bind（）、listen操作
        proxyDataPort_ = Utils::GetSockLocalPort(proxyListenDataSocket); //这个端口是代理服务器随机生成的
        proxyListenDataSocket_ = proxyListenDataSocket;
        auto proxyListenDataSocketEvent = Event::create(proxyListenDataSocket);
        proxyListenDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyListenDataReadCb(std::forward<decltype(PH1)>(PH1)); });
        epoll_->AddEvent(std::move(proxyListenDataSocketEvent));
        pasv_mode = 2;

        char param_temp[BUFFSIZE];
        strcpy(param_temp,param);
        clientDataPort_ = Utils::GetPortFromFtpParam(param_temp);
        bzero(buff,BUFFSIZE);
        sprintf(buff,"PORT %s,%d,%d\r\n",proxyIp_.c_str(),proxyDataPort_ / 256,proxyDataPort_ % 256);

    }else if(strcmp(cmd,CMD_PASV) == 0){
        lastCmdCount_ = CMD_PASS_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_TYPE) == 0){
        lastCmdCount_ = CMD_TYPE_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_STRU) == 0){
        lastCmdCount_ = CMD_STRU_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_MODE) == 0){
        lastCmdCount_ = CMD_MODE_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_RETR) == 0){
        lastCmdCount_ = CMD_RETR_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_STOR) == 0){
        lastCmdCount_ = CMD_STOR_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_APPE) == 0){
        lastCmdCount_ = CMD_APPE_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_ALLO) == 0){
        lastCmdCount_ = CMD_ALLO_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_REST) == 0){
        lastCmdCount_ = CMD_REST_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_RNFR) == 0){
        lastCmdCount_ = CMD_RNFR_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_RNTO) == 0){
        lastCmdCount_ = CMD_RNTO_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_ABOR) == 0){
        lastCmdCount_ = CMD_ABOR_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_DELE) == 0){
        lastCmdCount_ = CMD_DELE_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_MKD) == 0){
        lastCmdCount_ = CMD_MKD_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_PWD) == 0){
        lastCmdCount_ = CMD_PWD_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_LIST) == 0){
        lastCmdCount_ = CMD_LIST_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_NLST) == 0){
        lastCmdCount_ = CMD_NLST_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_SITE) == 0){
        lastCmdCount_ = CMD_SITE_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_SYST) == 0){
        lastCmdCount_ = CMD_SYST_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_STAT) == 0){
        lastCmdCount_ = CMD_STAT_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_HELP) == 0){
        lastCmdCount_ = CMD_HELP_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else if(strcmp(cmd,CMD_NOOP) == 0){
        lastCmdCount_ = CMD_NOOP_COUNT;
        DoNothingForCmd(buff,cmd,param);
    }else{
        DoNothingForCmd(buff,cmd,param);
    }


    write(proxyToServerCmdSocket_,buff,strlen(buff));

    return 0;
}


//这里完全根据服务端返回的状态码来判断是有问题的，不同的客户端命令可能返回相同的状态码，所有要结合客户端的命令来判断
//如果结合客户端命令来的话，那么if-else就比较多了。
int Client::ServerStatusHandle(char *cmd,char *param){

    char buff[BUFFSIZE];
    bzero(buff,BUFFSIZE);

    switch(lastCmdCount_){
        case CMD_USER_COUNT:
            break;
        case CMD_PASS_COUNT:
            break;
        case CMD_ACCT_COUNT:
            break;
        case CMD_CWD_COUNT:
            break;
        case CMD_CDUP_COUNT:
            break;
        case CMD_SMNT_COUNT:
            break;
        case CMD_QUIT_COUNT:
            break;
        case CMD_REIN_COUNT:
            break;
        case CMD_PORT_COUNT:
            //命令是PROT，返回的状态码是227，那么就应该没有问题
            if(strcmp(cmd,"227") == 0){
                pasv_mode = 1;
                int proxyListenDataSocket = Utils::BindAndListenSocket(0); //开启proxyListenDataSocket、bind（）、listen操作
                proxyListenDataSocket_ = proxyListenDataSocket;
                auto proxyListenDataSocketEvent = Event::create(proxyListenDataSocket);
                proxyListenDataSocketEvent->SetReadHandle([capture0 = GetClientPtr()](auto && PH1) { capture0->ProxyListenDataReadCb(std::forward<decltype(PH1)>(PH1)); });
                //这是本地随机生成的端口号
                proxyDataPort_ = Utils::GetSockLocalPort(proxyListenDataSocket);

                epoll_->AddEvent(std::move(proxyListenDataSocketEvent));

                serverDataPort_ = Utils::GetPortFromFtpParam(param + 23);
                bzero(param + 23,BUFFSIZE - 23);
                sprintf(param + 23,"%s,%d,%d).\r\n",clientIp_.c_str(),proxyDataPort_ / 256,proxyDataPort_ % 256);
            }
            break;
        case CMD_PASV_COUNT:
            break;
        case CMD_TYPE_COUNT:
            break;
        case CMD_STRU_COUNT:
            break;
        case CMD_MODE_COUNT:
            break;
        case CMD_RETR_COUNT:
            break;
        case CMD_STOR_COUNT:
            break;
        case CMD_APPE_COUNT:
            break;
        case CMD_ALLO_COUNT:
            break;
        case CMD_REST_COUNT:
            break;
        case CMD_RNFR_COUNT:
            break;
        case CMD_RNTO_COUNT:
            break;
        case CMD_ABOR_COUNT:
            break;
        case CMD_DELE_COUNT:
            break;
        case CMD_MKD_COUNT:
            break;
        case CMD_PWD_COUNT:
            break;
        case CMD_LIST_COUNT:
            break;
        case CMD_NLST_COUNT:
            break;
        case CMD_SITE_COUNT:
            break;
        case CMD_SYST_COUNT:
            break;
        case CMD_STAT_COUNT:
            break;
        case CMD_HELP_COUNT:
            break;
        case CMD_NOOP_COUNT:
            break;
        default:
            PROXY_LOG_FATAL("unknown cmd count");
    }

    sprintf(buff,"%s%s\r\n",cmd,param);
    write(clientToProxyCmdSocket_,buff,strlen(buff));
    return 0;
}


//现在是什么都不做，就先直接发送就可以了
int Client::ClientDataHandle(char *data) {
    //这个应该在那个联合数据里指定这个data的长度
    write(proxyToServerDataSocket_,data,strlen(data));
    return 0;
}

//这个也是直接发送
int Client::ServerDataHandle(char *data) {
    write(clientToProxyDataSocket_,data,strlen(data));
    return 0;
}

void Client::ProxyHandleData(const std::shared_ptr<SerializeProtoData>& serializeProtoData) {
    char magic = serializeProtoData->GetMagic();
    char type = serializeProtoData->GetType();
    Data data = serializeProtoData->GetData();

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
