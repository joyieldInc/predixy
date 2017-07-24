/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "AcceptSocket.h"


AcceptSocket::AcceptSocket(int fd, sockaddr* addr, socklen_t len):
    Socket(fd)
{
    mClassType = AcceptType;
    mPeer[0] = '\0';
    switch (addr->sa_family) {
    case AF_INET:
        {
            char host[INET_ADDRSTRLEN];
            sockaddr_in* in = (sockaddr_in*)addr;
            inet_ntop(AF_INET, (void*)&in->sin_addr, host, sizeof(host));
            int port = ntohs(in->sin_port);
            snprintf(mPeer, sizeof(mPeer), "%s:%d", host, port);
        }
        break;
    case AF_INET6:
        {
            char host[INET6_ADDRSTRLEN];
            sockaddr_in6* in = (sockaddr_in6*)addr;
            inet_ntop(AF_INET6, (void*)&in->sin6_addr, host, sizeof(host));
            int port = ntohs(in->sin6_port);
            snprintf(mPeer, sizeof(mPeer), "%s:%d", host, port);
        }
        break;
    case AF_UNIX:
        snprintf(mPeer, sizeof(mPeer), "unix");
        break;
    default:
        snprintf(mPeer, sizeof(mPeer), "unknown");
        break;
    }
}
