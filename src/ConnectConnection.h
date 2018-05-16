/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CONNECT_CONNECTION_H_
#define _PREDIXY_CONNECT_CONNECTION_H_

#include "Predixy.h"
#include "ConnectSocket.h"
#include "Connection.h"
#include "Request.h"
#include "ResponseParser.h"

class ConnectConnection :
    public ConnectSocket,
    public Connection,
    public ListNode<ConnectConnection>,
    public DequeNode<ConnectConnection>
{
public:
    typedef ConnectConnection Value;
    typedef ListNode<ConnectConnection> ListNodeType;
    typedef DequeNode<ConnectConnection> DequeNodeType;
    typedef Alloc<ConnectConnection, Const::ConnectConnectionAllocCacheSize> Allocator;
public:
    ConnectConnection(Server* s, bool shared);
    ~ConnectConnection();
    bool writeEvent(Handler* h);
    void readEvent(Handler* h);
    void close(Handler* h);
    void send(Handler* h, Request* req)
    {
        mSendRequests.push_back(req);
    }
    Server* server() const
    {
        return mServ;
    }
    bool isShared() const
    {
        return mShared;
    }
    bool isAuth() const
    {
        return mAuthed;
    }
    void setAuth(bool v)
    {
        mAuthed = v;
    }
    bool readonly() const
    {
        return mReadonly;
    }
    void setReadonly(bool v)
    {
        mReadonly = v;
    }
    void attachAcceptConnection(AcceptConnection* c)
    {
        mAcceptConnection = c;
    }
    void detachAcceptConnection()
    {
        mAcceptConnection = nullptr;
    }
    AcceptConnection* acceptConnection() const
    {
        return mAcceptConnection;
    }
    int pendRequestCount() const
    {
        return mSendRequests.size() + mSentRequests.size();
    }
    Request* frontRequest() const
    {
        return !mSentRequests.empty() ? mSentRequests.front() :
              (!mSendRequests.empty() ? mSendRequests.front() : nullptr);
    }
private:
    void parse(Handler* h, Buffer* buf, int pos);
    void handleResponse(Handler* h);
    void write();
    int fill(Handler* h, IOVec* bufs, int len);
    bool write(Handler* h, IOVec* bufs, int len);
private:
    Server* mServ;
    AcceptConnection* mAcceptConnection;
    ResponseParser mParser;
    SendRequestList mSendRequests;
    SendRequestList mSentRequests;
    bool mShared;
    bool mAuthed;
    bool mReadonly;
};

typedef List<ConnectConnection> ConnectConnectionList;
typedef Deque<ConnectConnection> ConnectConnectionDeque;
typedef ConnectConnection::Allocator ConnectConnectionAlloc;

#endif
