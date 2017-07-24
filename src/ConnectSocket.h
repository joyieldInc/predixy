/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CONNECT_SOCKET_H_
#define _PREDIXY_CONNECT_SOCKET_H_

#include "Socket.h"
#include <string>

class ConnectSocket : public Socket
{
public:
    enum ConnectStatus
    {
        Unconnected,
        Connecting,
        Connected,
        Disconnected
    };
public:
    ConnectSocket(const char* peer, int type, int protocol = 0);
    bool connect();
    void reopen();
    void close();
    const char* peer() const
    {
        return mPeer.c_str();
    }
    ConnectStatus connectStatus() const
    {
        return good() ? mStatus : Disconnected;
    }
    bool isConnecting() const
    {
        return mStatus == Connecting;
    }
    void setConnected()
    {
        mStatus = Connected;
    }
    void setConnectStatus(ConnectStatus st)
    {
        mStatus = st;
    }
private:
    std::string mPeer;
    sockaddr_storage mPeerAddr;
    socklen_t mPeerAddrLen;
    int mType;
    int mProtocol;
    ConnectStatus mStatus;
};


#endif
