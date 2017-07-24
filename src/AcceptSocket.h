/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_ACCEPT_SOCKET_H_
#define _PREDIXY_ACCEPT_SOCKET_H_

#include "Socket.h"

class AcceptSocket : public Socket
{
public:
    AcceptSocket(int fd, sockaddr* addr, socklen_t len);
    const char* peer() const
    {
        return mPeer;
    }
private:
    char mPeer[32];
};


#endif
