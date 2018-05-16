/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_RESPONSE_H_
#define _PREDIXY_RESPONSE_H_

#include "Predixy.h"
#include "ResponseParser.h"

class Response :
    public TID<Response>,
    public ListNode<Response, SharePtr<Response>>,
    public RefCntObj<Response>
{
public:
    typedef Response Value;
    typedef ListNode<Response, SharePtr<Response>> ListNodeType;
    typedef Alloc<Response, Const::ResponseAllocCacheSize> Allocator;
    enum GenericCode
    {
        Pong,
        Ok,
        Cmd,
        UnknownCmd,
        ArgWrong,
        InvalidDb,
        NoPasswordSet,
        InvalidPassword,
        Unauth,
        PermissionDeny,
        NoServer,
        NoServerConnection,
        ServerConnectionClose,
        DeliverRequestFail,
        ForbidTransaction,
        ConfigSubCmdUnknown,
        InvalidScanCursor,
        ScanEnd,

        CodeSentinel
    };
    static void init();
    static Response* create(GenericCode code, Request* req = nullptr);
public:
    Response();
    Response(GenericCode code);
    ~Response();
    void set(const ResponseParser& p);
    void set(int64_t num);
    void setStr(const char* str, int len = -1);
    void setErr(const char* str, int len = -1);
    void adjustForLeader(Request* req);
    bool send(Socket* s);
    int fill(IOVec* vecs, int len, Request* req) const;
    void setType(Reply::Type t)
    {
        mType = t;
    }
    Reply::Type type() const
    {
        return mType;
    }
    const char* typeStr() const
    {
        return Reply::TypeStr[mType];
    }
    int64_t integer() const
    {
        return mInteger;
    }
    bool isOk() const
    {
        return mType == Reply::Status && mRes.hasPrefix("+OK");
    }
    bool isPong() const
    {
        return mType == Reply::Status && mRes.hasPrefix("+PONG");
    }
    bool isError() const
    {
        return mType == Reply::Error;
    }
    bool isInteger() const
    {
        return mType == Reply::Integer;
    }
    bool isString() const
    {
        return mType == Reply::String;
    }
    bool isArray() const
    {
        return mType == Reply::Array;
    }
    bool isMoved() const
    {
        return mType == Reply::Error && mRes.hasPrefix("-MOVED ");
    }
    bool getMoved(int& slot, SString<Const::MaxAddrLen>& addr) const
    {
        return getAddr(slot, addr, "-MOVED ");
    }
    bool isAsk() const
    {
        return mType == Reply::Error && mRes.hasPrefix("-ASK ");
    }
    bool getAsk(SString<Const::MaxAddrLen>& addr) const
    {
        int slot;
        return getAddr(slot, addr, "-ASK ");
    }
    Segment& head()
    {
        return mHead;
    }
    const Segment& head() const
    {
        return mHead;
    }
    const Segment& body() const
    {
        return mRes;
    }
    Segment& body()
    {
        return mRes;
    }
private:
    bool getAddr(int& slot, SString<Const::MaxAddrLen>& addr, const char* token) const;
private:
    Reply::Type mType;
    int64_t mInteger;
    Segment mHead; //for mget
    Segment mRes;
};

typedef List<Response> ResponseList;
typedef Response::Allocator ResponseAlloc;

#endif
