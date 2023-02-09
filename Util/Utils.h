//
// Created by tomatoo on 12/26/22.
//

#ifndef FTP_PROXY_UTILS_H
#define FTP_PROXY_UTILS_H

#include <sys/socket.h>
#include <memory>
#include <string>

//继承这个就只能通过create创建了
template<typename T>
class unique_ptr_only{
public:
    typedef std::unique_ptr<T> uniquePtr;

    template<typename... Args>
    static typename unique_ptr_only<T>::uniquePtr create(Args&&... args){
        return std::unique_ptr<T>(new T(args...));
    }

};

template<typename T>
class shared_ptr_only{
public:
    typedef std::shared_ptr<T> sharedPtr;

    template<typename... Args>
    static typename shared_ptr_only<T>::sharedPtr create(Args&&... args){
        return std::shared_ptr<T>(new T(args...));
    }

};


class Utils{
public:
    static int AcceptSocket(int socket,struct sockaddr *addr,socklen_t *addrlen);
    static int ConnectToServerByAddr(struct sockaddr_in servaddr);
    static int ConnectToServer(const std::string& ip,unsigned short port);
    static int BindAndListenSocket(unsigned short port);
    static void SplitCmd(char *buff, char *cmd, const char *param);
    static void SplitStatus(char *buff, char *cmd, const char *param);
    static unsigned short GetPortFromFtpParam(char *param);
    static void GetSockLocalIp(int fd,std::string &ip);
    static unsigned short GetSockLocalPort(int sockfd);

    static int Readn(int sockfd,char *buff,int len);
    static int Writen(int sockfd,const char *buff,int len);
};

#endif //FTP_PROXY_UTILS_H
