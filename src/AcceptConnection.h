/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_ACCEPT_CONNECTION_H_
#define _PREDIXY_ACCEPT_CONNECTION_H_

#include "Predixy.h"
#include "AcceptSocket.h"
#include "Connection.h"
#include "Transaction.h"
#include "Subscribe.h"
#include "RequestParser.h"
#include "Request.h"
#include "Response.h"

class AcceptConnection :
    public AcceptSocket,
    public Connection,
    public Transaction,
    public Subscribe,
    public ListNode<AcceptConnection, SharePtr<AcceptConnection>>,
    public DequeNode<AcceptConnection, SharePtr<AcceptConnection>>,
    public RefCntObj<AcceptConnection, AtomicInt>
{
public:
    typedef AcceptConnection Value;
    typedef ListNode<AcceptConnection, SharePtr<AcceptConnection>> ListNodeType;
    typedef DequeNode<AcceptConnection, SharePtr<AcceptConnection>> DequeNodeType;
    typedef Alloc<AcceptConnection, Const::AcceptConnectionAllocCacheSize> Allocator;
public:
    AcceptConnection(int fd, sockaddr* addr, socklen_t len);
    ~AcceptConnection();
    void close();
    bool writeEvent(Handler* h);
    void readEvent(Handler* h);
    bool send(Handler* h, Request* req, Response* res);
    long lastActiveTime() const
    {
        return mLastActiveTime;
    }
    void setLastActiveTime(long v)
    {
        mLastActiveTime = v;
    }
    bool empty() const
    {
        return mRequests.empty();
    }
    ConnectConnection* connectConnection() const
    {
        return mConnectConnection;
    }
    void attachConnectConnection(ConnectConnection* s)
    {
        mConnectConnection = s;
    }
    void detachConnectConnection()
    {
        mConnectConnection = nullptr;
    }
    const Auth* auth() const
    {
        return mAuth;
    }
    void setAuth(const Auth* auth)
    {
        mAuth = auth;
    }
    bool isBlockRequest() const
    {
        return mBlockRequest;
    }
    void setBlockRequest(bool v)
    {
        mBlockRequest = v;
    }
    void append(Request* req)
    {
        mRequests.push_back(req);
    }
private:
    void parse(Handler* h, Buffer* buf, int pos);
    int fill(Handler* h, IOVec* bufs, int len);
    bool write(Handler* h, IOVec* bufs, int len);
private:
    RequestParser mParser;
    RecvRequestList mRequests;
    RequestPtr mReqLeader;
    ResponseList mResponses;
    const Auth* mAuth;
    ConnectConnection* mConnectConnection;
    long mLastActiveTime; //steady us
    bool mBlockRequest;
};

typedef List<AcceptConnection> AcceptConnectionList;
typedef Deque<AcceptConnection> AcceptConnectionDeque;
typedef AcceptConnection::Allocator AcceptConnectionAlloc;

#endif
