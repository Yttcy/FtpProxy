//
// Created by tomatoo on 1/12/23.
//

#include "FtpTcpBuffer.h"
#include "Log.h"
#include <algorithm>
#include <memory.h>


FtpTcpBuffer::FtpTcpBuffer():
buffer_(),
index_(0)
{

}

//这里会不会有点慢,算了，先慢着吧
FtpTcpBuffer& FtpTcpBuffer::operator += (char *buff) {
    size_t len = strlen(buff);

    size_t originSize = buffer_.size();
    buffer_.resize(originSize + len);
    memcpy(buffer_.data() + originSize, buff, len);

    return *this;
}

int FtpTcpBuffer::JudgeCmd(){
    int length = buffer_.size();
    for(size_t i = index_; i < length; ++i){
        if(buffer_[i] == '\n'){
            if(i > 0 && buffer_[i-1] == '\r'){
                index_ = i+1 ;
                return 0;
            }
        }
    }
    index_ = length;
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


int FtpTcpBuffer::JudgeStatus(){
    auto position = std::find(buffer_.begin()+index_,buffer_.end(),'\n');
    if(position == buffer_.begin() || *(position-1) != '\r'){
        index_ = position - buffer_.begin() + 1;
        position = buffer_.end();
    }
    if(position == buffer_.end()){
        return -1;
    }else if(position - buffer_.begin() > 3 && JudgeStatusPart()){  //如果多行内容只是的某一行\r\n
        index_ = position - buffer_.begin() + 1;
        return 0;
    }else{
        index_ = position - buffer_.begin()+ 1;
        return JudgeStatus();
    }
}

void FtpTcpBuffer::Print() {
    for(auto i:buffer_){
        printf("%c",i);
    }
}