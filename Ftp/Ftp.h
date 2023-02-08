//
// Created by tomatoo on 2/1/23.
//

#ifndef FTP_PROXY_FTP_H
#define FTP_PROXY_FTP_H


#include "Event.h"
#include <memory>

class Ftp :public std::enable_shared_from_this<Ftp>{
public:
    explicit Ftp();
    int AddToLoop(std::shared_ptr<EventLoop> &loop);
    int DelFromLoop();
private:
    void FtpEvent(int sockfd);

private:
    int listenCmdSocket_{};
    std::shared_ptr<EventLoop> loop_;
};


#endif //FTP_PROXY_FTP_H
