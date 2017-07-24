/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "ConnectSocket.h"


ConnectSocket::ConnectSocket(const char* peer, int type, int protocol):
    mPeer(peer),
    mType(type),
    mProtocol(protocol)
{
    mClassType = ConnectType;
    mPeerAddrLen = sizeof(mPeerAddr);
    getFirstAddr(peer, type, protocol, (sockaddr*)&mPeerAddr, &mPeerAddrLen);
    sockaddr* in = (sockaddr*)&mPeerAddr;
    int fd = Socket::socket(in->sa_family, type, protocol);
    attach(fd);
    mStatus = Unconnected;
}

bool ConnectSocket::connect()
{
    if (mStatus == Connecting || mStatus == Connected) {
        return true;
    }
    bool retry;
    do {
        retry = false;
        int ret = ::connect(fd(), (const sockaddr*)&mPeerAddr, mPeerAddrLen);
        if (ret == 0) {
            mStatus = Connected;
        } else {
            if (errno == EINPROGRESS || errno == EALREADY) {
                mStatus = Connecting;
            } else if (errno == EISCONN) {
                mStatus = Connected;
            } else if (errno == EINTR) {
                retry = true;
            } else {
                mStatus = Unconnected;
                return false;
            }
        }
    } while (retry);
    return true;
}

void ConnectSocket::reopen()
{
    if (fd() >= 0) {
        return;
    }
    sockaddr* in = (sockaddr*)&mPeerAddr;
    int fd = Socket::socket(in->sa_family, mType, mProtocol);
    attach(fd);
    mStatus = Unconnected;
}

void ConnectSocket::close()
{
    mStatus = Disconnected;
    Socket::close();
}
