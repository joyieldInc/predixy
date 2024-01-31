/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_REQUEST_H_
#define _PREDIXY_REQUEST_H_

#include "Predixy.h"

enum RequestListIndex
{
    Recv = 0,
    Send,

    Size
};

class Request :
    public TID<Request>,
    public ListNode<Request, SharePtr<Request>, RequestListIndex::Size>,
    public RefCntObj<Request>
{
public:
    typedef Request Value;
    typedef ListNode<Request, SharePtr<Request>, RequestListIndex::Size> ListNodeType;
    typedef Alloc<Request, Const::RequestAllocCacheSize> Allocator;
    static const int MaxRedirectLimit = 3;
    enum GenericCode
    {
        Ping,
        PingServ,
        ClusterNodes,
        Asking,
        Readonly,
        UnwatchServ,
        DiscardServ,
        MgetHead,
        MsetHead,
        MsetnxHead,
        TouchHead,
        ExistsHead,
        DelHead,
        UnlinkHead,
        PsubscribeHead,
        SubscribeHead,
        PunsubscribeHead,
        UnsubscribeHead,

        CodeSentinel
    };
    static void init();
public:
    Request();
    Request(AcceptConnection* c);
    Request(GenericCode code);
    ~Request();
    void clear();
    void set(const RequestParser& p, Request* leader = nullptr);
    void setAuth(const String& password);
    void setSelect(int db);
    void setSentinels(const String& master);
    void setSentinelGetMaster(const String& master);
    void setSentinelSlaves(const String& master);
    void adjustScanCursor(long cursor);
    void follow(Request* leader);
    void setResponse(Response* res);
    bool send(Socket* s);
    int fill(IOVec* vecs, int len);
    bool isDone() const;
    AcceptConnection* connection() const
    {
        return mConn;
    }
    void detach()
    {
        mConn = nullptr;
    }
    void setType(Command::Type t)
    {
        mType = t;
    }
    Command::Type type() const
    {
        return mType;
    }
    const char* cmd() const
    {
        return Command::get(mType).name;
    }
    bool isInner() const
    {
        return Command::get(mType).mode & Command::Inner;
    }
    const Segment& key() const
    {
        return mKey;
    }
    const Segment& body() const
    {
        return mReq;
    }
    void setData(void* dat)
    {
        mData = dat;
    }
    void* data() const
    {
        return mData;
    }
    void rewind()
    {
        mHead.rewind();
        mReq.rewind();
    }
    Response* getResponse() const
    {
        return mRes;
    }
    Request* leader() const
    {
        return isLeader() ? const_cast<Request*>(this) : (Request*)mLeader;
    }
    bool isLeader() const
    {
        return mFollowers > 0;
    }
    bool isDelivered() const
    {
        return mDelivered;
    }
    bool isInline() const
    {
        return mInline;
    }
    void setDelivered()
    {
        mDelivered = true;
    }
    int followers() const
    {
        return mFollowers;
    }
    int redirectCnt() const
    {
        return mRedirectCnt;
    }
    int incrRedirectCnt()
    {
        return ++mRedirectCnt;
    }
    bool requireWrite() const
    {
        return Command::get(mType).mode & Command::Write;
    }
    bool requirePrivateConnection() const
    {
        return Command::get(mType).mode & Command::Private;
    }
    long createTime() const
    {
        return mCreateTime;
    }
private:
    AcceptConnection* mConn;
    Command::Type mType;
    ResponsePtr mRes;
    bool mDone;
    bool mDelivered;
    bool mInline;
    Segment mHead; //for multi key command mget/mset/del...
    Segment mReq;
    Segment mKey;
    RequestPtr mLeader;
    int mFollowers;
    int mFollowersDone;
    int mRedirectCnt;
    long mCreateTime; //steady time point us
    void* mData; //user data for response
};

typedef List<Request, RequestListIndex::Recv> RecvRequestList;
typedef List<Request, RequestListIndex::Send> SendRequestList;
typedef Request::Allocator RequestAlloc;

#endif
