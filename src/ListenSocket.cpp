/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <string.h>
#include "Util.h"
#include "ListenSocket.h"

ListenSocket::ListenSocket(const char* addr, int type, int protocol)
{
    mClassType = ListenType;
    strncpy(mAddr, addr, sizeof(mAddr));
    sockaddr_storage saddr;
    socklen_t len = sizeof(saddr);
    getFirstAddr(addr, type, protocol, (sockaddr*)&saddr, &len);
    sockaddr* in = (sockaddr*)&saddr;
    int fd = Socket::socket(in->sa_family, type, protocol);
    int flags = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    int ret = ::bind(fd, in, len);
    if (ret != 0) {
        int err = errno;
        ::close(fd);
        errno = err;
        Throw(BindFail, "socket bind to %s fail %s", addr, StrError());
    }
    attach(fd);
}

void ListenSocket::listen(int backlog)
{
    int ret = ::listen(fd(), backlog);
    if (ret != 0) {
        Throw(ListenFail, "socket listen fail %s", StrError());
    }
}

int ListenSocket::accept(sockaddr* addr, socklen_t* len)
{
    while (true) {
        int c = ::accept(fd(), addr, len);
        if (c < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return -1;
            } else if (errno == EINTR || errno == ECONNABORTED) {
                continue;
            } else if (errno == EMFILE || errno == ENFILE) {
                Throw(TooManyOpenFiles, "socket accept fail %s", StrError());
            }
            Throw(AcceptFail, "socket accept fail %s", StrError());
        }
        return c;
    }
    return -1;
}
