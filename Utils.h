//
// Created by tomatoo on 12/26/22.
//

#ifndef FTP_PROXY_UTILS_H
#define FTP_PROXY_UTILS_H

#include <sys/socket.h>
#include <memory>
#include <string>


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
};


#endif //FTP_PROXY_UTILS_H
