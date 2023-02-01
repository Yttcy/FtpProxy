//
// Created by tomatoo on 1/4/23.
//

#include "Proto.h"

SerializeProtoData::SerializeProtoData(char magic,char type,Data s):
type_(type),
magic_(magic),
info_(s)
{

}

char SerializeProtoData::GetMagic() const {
    return magic_;
}

char SerializeProtoData::GetType() const {
    return type_;
}

Data SerializeProtoData::GetData() {
    return info_;
}


