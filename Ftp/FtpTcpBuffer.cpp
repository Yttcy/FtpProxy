//
// Created by tomatoo on 1/12/23.
//

#include "FtpTcpBuffer.h"


FtpTcpBuffer::FtpTcpBuffer():
buffer_(),
index_(0)
{

}

FtpTcpBuffer& FtpTcpBuffer::operator+=(char *buff) {
    buffer_ += buff;
    return *this;
}

int FtpTcpBuffer::JudgeCmd() {
    int length = buffer_.length();
    for(size_t i = index_; i < length; ++i ){
        if(buffer_[i] == '\n'){
            if(i > 0 && buffer_[i-1] == '\r'){
                index_ = i+1 ;
                return 0;
            }
        }
    }
    index_ = length ;

    return -1;
}



std::string FtpTcpBuffer::GetCompleteCmd(){
    auto end = buffer_.begin() + index_;
    std::string result(buffer_.begin(),end);
    buffer_.erase(buffer_.begin(),end);
    index_ = 0;
    return result;
}


//和GetCmd是一样的
std::string FtpTcpBuffer::GetCompleteStatus() {
    return GetCompleteCmd();
}

bool FtpTcpBuffer::JudgeStatusPart(){
    for(int i=0;i<3;++i){
        if(buffer_[index_+i] < '0' || buffer_[index_ + i] > '9'){
            return false;
        }
    }

    if(buffer_[index_ + 3] != ' '){
        return false;
    }

    return true;
}


int FtpTcpBuffer::JudgeStatus() {

    size_t position = buffer_.find_first_of("\r\n",index_);

    if(position == std::string::npos){
        return -1;
    }else if(position - index_ > 3 && JudgeStatusPart()){  //如果多行内容只是的某一行\r\n
        index_ = position + 2;
        return 0;
    }else{
        index_ = position + 2;
        return JudgeStatus();
    }
}