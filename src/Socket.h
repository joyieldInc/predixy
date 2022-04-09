/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SOCKET_H_
#define _PREDIXY_SOCKET_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "Exception.h"
#include "Logger.h"

class Socket
{
public:
    DefException(SocketCallFail);
    DefException(InvalidAddr);
    DefException(AddrLenTooShort);

    enum ClassType
    {
        RawType,
        ListenType,
        AcceptType,
        ConnectType,

        CustomType = 1024
    };
    enum StatusCode
    {
        Normal = 0,
        None,
        End,
        IOError,
        EventError,
        ExceptError,

        CustomStatus = 100
    };
public:
    Socket(int fd = -1);
    Socket(int domain, int type, int protocol = 0);
    Socket(const Socket&) = delete;
    Socket(const Socket&&) = delete;
    ~Socket();
    void attach(int fd);
    void detach();
    void close();
    bool setNonBlock(bool val = true);
    bool setTcpNoDelay(bool val = true);
    bool setTcpKeepAlive(int interval);
    int read(void* buf, int cnt);
    int write(const void* buf, int cnt);
    int writev(const struct iovec* vecs, int cnt);
    int classType() const
    {
        return mClassType;
    }
    int fd() const
    {
        return mFd;
    }
    bool good() const
    {
        return mStatus == Normal;
    }
    int status() const
    {
        return mStatus;
    }
    const char* statusStr() const;
    void setStatus(int st)
    {
        logError("ibk: SET_STATUS setStatus %d", st);
        mStatus = st;
    }
    int getEvent() const
    {
        return mEvts;
    }
    void setEvent(int evts)
    {
        mEvts = evts;
    }
public:
    static int socket(int domain, int type, int protocol);
    static void getFirstAddr(const char* addr, int type, int protocol, sockaddr* res, socklen_t* len);
protected:
    int mClassType;
private:
    int mFd;
    int mEvts;
    int mStatus;
};

#endif
