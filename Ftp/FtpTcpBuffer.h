//
// Created by tomatoo on 1/12/23.
//

#ifndef FTP_PROXY_TCPBUFFER_H
#define FTP_PROXY_TCPBUFFER_H

#include <string>
#include <vector>

class FtpTcpBuffer {

public:
    explicit FtpTcpBuffer();

    FtpTcpBuffer& operator += (char *);

    std::string GetCompleteCmd();

    std::string GetCompleteStatus();

    int JudgeCmd();

    int JudgeStatus();

    void Print();

private:
    bool JudgeStatusPart();

private:
    std::vector<char> buffer_; //TCP 缓冲区
    int index_;
};


#endif //FTP_PROXY_TCPBUFFER_H
