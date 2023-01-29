//
// Created by tomatoo on 12/26/22.
//

#include "Utils.h"
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <memory>

#include <PublicParameters.h>

#define LISTEN_NUMBER 5
#define IP_SIZE 100


unsigned short Utils::GetSockLocalPort(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    if(getsockname(sockfd,(struct sockaddr *)&addr,&addrlen) < 0){
        perror("getsockname() failed: ");
        exit(1);
    }

    return ntohs(addr.sin_port);
}


void Utils::GetSockLocalIp(int fd,std::string& ip)
{


    struct sockaddr_in addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    if(getsockname(fd,(struct sockaddr *)&addr,&addrlen) < 0){
        perror("getsockname() failed: ");
        exit(1);
    }

    char ipStr[IP_SIZE];
    inet_ntop(AF_INET,&addr.sin_addr,ipStr,addrlen);

    char *p = ipStr;
    while(*p){
        if(*p == '.')
            *p = ',';
        p++;
    }
    ip = ipStr;

}


unsigned short Utils::GetPortFromFtpParam(char *param)
{
    unsigned short port,t;
    int count = 0;
    char *p = param;

    while(count < 4){
        if(*(p++) == ','){
            count++;
        }
    }

    sscanf(p,"%hu",&port);
    while(*p != ',' && *p != '\r' && *p != ')')
        p++;


    if(*p == ','){
        p++;
        sscanf(p,"%hu",&t);
        port = port * 256 + t;
    }
    return port;
}

//从FTP命令行中解析出命令和参数
void Utils::SplitCmd(char *buff, char *cmd, const char *param)
{
    int i;
    char *p;

    //这里就是如果最后的两个char类型是/r 或者 是/n 那么就把这个弄为0
    while((p = &buff[strlen(buff) - 1]) && (*p == '\r' || *p == '\n'))
        *p = 0;

    char *cmd_temp = strtok(buff," ");

    //如果客户端传来的命令长度大于4，就先将命令设置位WHAT返回，后续在处理
    if(strlen(cmd_temp) > 4){
        memcpy((void*)cmd,"WHAT",COMMAND_MAX_LENGTH);
        return;
    }else{
        memcpy(cmd,cmd_temp,strlen(cmd_temp));
    }


    char *param_temp = strtok(nullptr," ");
    if(param_temp != nullptr){
        memcpy((void*)param,param_temp,strlen(param_temp));
    }

    //将主命令全部弄成大写
    char *cmd_ex = cmd;
    for(i = 0;i < strlen(cmd_ex);i++){
        cmd_ex[i] = toupper(cmd_ex[i]);
    }
}

void Utils::SplitStatus(char *buff, char *cmd,const char *param) {
    char *p;
    //这里就是如果最后的两个char类型是/r 或者 是/n 那么就把这个弄为0
    while((p = &buff[strlen(buff) - 1]) && (*p == '\r' || *p == '\n'))
        *p = 0;


    memcpy((void*)cmd,buff,3);
    memcpy((void*)param,buff+3,strlen(buff)-3);

}


int Utils::AcceptSocket(int cmd_socket,struct sockaddr *addr,socklen_t *addrlen)
{
    int fd = accept(cmd_socket,addr,addrlen);
    if(fd < 1){
        perror("accept() failed:");
        exit(1);
    }

    return fd;
}

int Utils::ConnectToServerByAddr(struct sockaddr_in servaddr)
{
    int fd;

    struct sockaddr_in cliaddr;
    bzero(&cliaddr,sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //cliaddr.sin_port = htons(20);

    fd = socket(AF_INET,SOCK_STREAM,0);
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR, nullptr,0);
    if(fd < 0){
        perror("socket() failed :");
        exit(1);
    }

    if(bind(fd,(struct sockaddr *)&cliaddr,sizeof(cliaddr) ) < 0){
        perror("bind() failed :");
        exit(1);
    }

    servaddr.sin_family = AF_INET;
    if(connect(fd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){
        perror("connect() failed :");
        exit(1);
    }

    return fd;
}


int Utils::ConnectToServer(const std::string& ip,unsigned short port)
{
    int fd;
    struct sockaddr_in servaddr;

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET,ip.c_str(),&servaddr.sin_addr);

    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd < 0){
        perror("socket() failed :");
        exit(1);
    }

    if(connect(fd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){
        perror("connect() failed :");
        exit(1);
    }

    return fd;
}

int Utils::BindAndListenSocket(unsigned short port)
{
    int fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET,SOCK_STREAM,0);
    int val;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));

    if(fd < 0){
        perror("socket() failed: ");
        exit(1);
    }

    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(fd,(struct sockaddr*)&addr,sizeof(addr)) < 0){
        perror("bind() failed: ");
        exit(1);
    }

    if(listen(fd,LISTEN_NUMBER) < 0){
        perror("listen() failed: ");
        exit(1);
    }

    return fd;
}
