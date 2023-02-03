//
// Created by tomatoo on 1/4/23.
//

#ifndef FTP_PROXY_PROTO_H
#define FTP_PROXY_PROTO_H

#include <cstddef>
#include <string>
#include <memory>

#include "PublicParameters.h"
#include "FtpTcpBuffer.h"

struct Data{
    union{
        struct{
            char cmd[COMMAND_MAX_LENGTH];
            char param[BUFFSIZE];
        }cmd;

        struct{
            char status[STATUS_MAX_LENGTH];
            char param[BUFFSIZE];
        }status;

        struct{
            char data[BUFFSIZE];
        }cdata;

        struct{
            char data[BUFFSIZE];
        }sdata;
    }u;
};

//封装的数据
class SerializeProtoData{
public:

    explicit SerializeProtoData(char magic,char type,Data info);

    char GetMagic() const;

    char GetType() const;

    Data GetData();

private:
    char magic_; //标识会话的主动发起方，或者是被动接收方
    char type_; //标识是数据传输，还是命令传输和状态码
    Data info_; //主要的数据,这个是可能为空的
};


#endif //FTP_PROXY_PROTO_H
