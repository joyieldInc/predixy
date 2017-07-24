/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_LISTEN_SOCKET_H_
#define _PREDIXY_LISTEN_SOCKET_H_

#include "Socket.h"

class ListenSocket : public Socket
{
public:
     DefException(BindFail);
     DefException(ListenFail);
     DefException(TooManyOpenFiles);
     DefException(AcceptFail);
public:
    ListenSocket(const char* addr, int type, int protocol = 0);
    void listen(int backlog = 511);
    int accept(sockaddr* addr, socklen_t* len);
    const char* addr() const
    {
        return mAddr;
    }
private:
    char mAddr[128];
};

#endif
