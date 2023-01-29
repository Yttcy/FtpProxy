//
// Created by tomatoo on 1/12/23.
//

#ifndef FTP_PROXY_TCPBUFFER_H
#define FTP_PROXY_TCPBUFFER_H

#include <string>

class TcpBuffer {

public:
    explicit TcpBuffer();

    TcpBuffer& operator += (char *);

    std::string GetCompleteCmd();

    std::string GetCompleteStatus();

    int JudgeCmd();

    int JudgeStatus();

private:
    bool JudgeStatusPart();

private:
    std::string buffer_; //TCP 缓冲区
    int index_;
};


#endif //FTP_PROXY_TCPBUFFER_H
