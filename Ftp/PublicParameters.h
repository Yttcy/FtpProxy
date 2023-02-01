//
// Created by tomatoo on 1/9/23.
//

#ifndef FTP_PROXY_PUBLICPARAMETERS_H
#define FTP_PROXY_PUBLICPARAMETERS_H

//标识是客户端还是服务端
#define MAGIC_CLIENT 'C'
#define MAGIC_SERVER 'S'


//标识是数据，还是客户端命令，还是服务端返回的状态码
#define TYPE_DATA 'D'
#define TYPE_CMD 'C'
#define TYPE_STATUS 'S'


//代理服务器的监听端口
#define PROXY_LISTEN_CMD_PORT 10000
#define SERVER_CMD_PORT 21

#define ServerIP "127.0.0.1"

//定义客户端的状态
#define STATUS_CONNECTED 0
#define STATUS_USER_INPUTTED 1
#define STATUS_PASS_AUTHENTICATED 2


//最大的客户端命令长度，需要+1
#define COMMAND_MAX_LENGTH 5

//响应码的长度 都是百位数 也需要+1
#define STATUS_MAX_LENGTH 4

//读缓冲区最大大小
#define BUFFSIZE 1024



//下面是FTP命令类型
/*******************************************************************************/
#define CMD_USER "USER"
#define CMD_PASS "PASS"
#define CMD_ACCT "ACCT"
#define CMD_CWD  "CWD"
#define CMD_CDUP "CDUP"
#define CMD_SMNT "SMNT"
#define CMD_QUIT "QUIT"
#define CMD_REIN "REIN"
#define CMD_PORT "PORT"
#define CMD_PASV "PASV"
#define CMD_TYPE "TYPE"
#define CMD_STRU "STRU"
#define CMD_MODE "MODE"
#define CMD_RETR "RETR"
#define CMD_STOR "STOR"
#define CMD_APPE "APPE"
#define CMD_ALLO "ALLO"
#define CMD_REST "REST"
#define CMD_RNFR "RNFR"
#define CMD_RNTO "RNTO"
#define CMD_ABOR "ABOR"
#define CMD_DELE "DELE"
#define CMD_MKD  "MKD"
#define CMD_PWD  "PWD"
#define CMD_LIST "LIST"
#define CMD_NLST "NLST"
#define CMD_SITE "SITE"
#define CMD_SYST "SYST"
#define CMD_STAT "STAT"
#define CMD_HELP "HELP"
#define CMD_NOOP "NOOP"

#define CMD_USER_COUNT 0
#define CMD_PASS_COUNT 1
#define CMD_ACCT_COUNT 2
#define CMD_CWD_COUNT  3
#define CMD_CDUP_COUNT 4
#define CMD_SMNT_COUNT 5
#define CMD_QUIT_COUNT 6
#define CMD_REIN_COUNT 7
#define CMD_PORT_COUNT 8
#define CMD_PASV_COUNT 9
#define CMD_TYPE_COUNT 10
#define CMD_STRU_COUNT 11
#define CMD_MODE_COUNT 12
#define CMD_RETR_COUNT 13
#define CMD_STOR_COUNT 14
#define CMD_APPE_COUNT 15
#define CMD_ALLO_COUNT 16
#define CMD_REST_COUNT 17
#define CMD_RNFR_COUNT 18
#define CMD_RNTO_COUNT 19
#define CMD_ABOR_COUNT 20
#define CMD_DELE_COUNT 21
#define CMD_MKD_COUNT  22
#define CMD_PWD_COUNT  23
#define CMD_LIST_COUNT 24
#define CMD_NLST_COUNT 25
#define CMD_SITE_COUNT 26
#define CMD_SYST_COUNT 27
#define CMD_STAT_COUNT 28
#define CMD_HELP_COUNT 29
#define CMD_NOOP_COUNT 30




/*******************************************************************************/

#endif //FTP_PROXY_PUBLICPARAMETERS_H
