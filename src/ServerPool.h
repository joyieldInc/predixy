/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SERVER_POOL_H_
#define _PREDIXY_SERVER_POOL_H_

#include <string>
#include <map>
#include "Predixy.h"

#include <vector>

class ServerPool
{
public:
    enum Type
    {
        Unknown,
        Cluster,
        Standalone
    };
    static const int DefaultServerRetryTimeout = 10000000;
    static const int DefaultRefreshInterval = 1000000;
public:
    virtual ~ServerPool();
    void init(const ServerPoolConf& conf);
    Proxy* proxy() const
    {
        return mProxy;
    }
    bool refresh();
    int type() const
    {
        return mType;
    }
    const String& password() const
    {
        return mPassword;
    }
    int masterReadPriority() const
    {
        return mMasterReadPriority;
    }
    int staticSlaveReadPriority() const
    {
        return mStaticSlaveReadPriority;
    }
    int dynamicSlaveReadPriority() const
    {
        return mDynamicSlaveReadPriority;
    }
    long refreshInterval() const
    {
        return mRefreshInterval;
    }
    long serverTimeout() const
    {
        return mServerTimeout;
    }
    int serverFailureLimit() const
    {
        return mServerFailureLimit;
    }
    long serverRetryTimeout() const
    {
        return mServerRetryTimeout;
    }
    int keepalive() const
    {
        return mKeepAlive;
    }
    int dbNum() const
    {
        return mDbNum;
    }
    ServerGroup* getGroup(int idx)
    {
        return idx < (int)mGroupPool.size() ? mGroupPool[idx] : nullptr;
    }
    Server* getServer(const String& addr)
    {
        UniqueLock<Mutex> lck(mMtx);
        auto it = mServs.find(addr);
        return it == mServs.end() ? nullptr : it->second;
    }
    Server* getServer(Handler* h, Request* req, const String& key) const
    {
        return mGetServerFunc(this, h, req, key);
    }
    Server* iter(int& cursor) const
    {
        return mIterFunc(this, cursor);
    }
    void refreshRequest(Handler* h)
    {
        mRefreshRequestFunc(this, h);
    }
    void handleResponse(Handler* h, ConnectConnection* s, Request* req, Response* res);
protected:
    typedef Server* (*GetServerFunc)(const ServerPool* p, Handler* h, Request* req, const String& key);
    typedef Server* (*IterFunc)(const ServerPool* p, int& cursor);
    typedef void (*RefreshRequestFunc)(ServerPool* p, Handler* h);
    typedef void (*HandleResponseFunc)(ServerPool* p, Handler* h, ConnectConnection* s, Request* req, Response* res);
    template<class T>
    ServerPool(Proxy* p, int type, T* sub = nullptr):
        mProxy(p),
        mType(type),
        mGetServerFunc(T::getServer),
        mIterFunc(T::iter),
        mRefreshRequestFunc(T::refreshRequest),
        mHandleResponseFunc(T::handleResponse),
        mLastRefreshTime(0),
        mMasterReadPriority(50),
        mStaticSlaveReadPriority(0),
        mDynamicSlaveReadPriority(0),
        mRefreshInterval(DefaultRefreshInterval),
        mServerFailureLimit(10),
        mServerRetryTimeout(DefaultServerRetryTimeout),
        mDbNum(1)
    {
    }
    static Server* randServer(Handler* h, const std::vector<Server*>& servs);
    static Server* iter(const std::vector<Server*>& servs, int& cursor);
protected:
    std::map<String, Server*> mServs;
    std::vector<ServerGroup*> mGroupPool;
private:
    Proxy* mProxy;
    int mType;
    GetServerFunc mGetServerFunc;
    IterFunc mIterFunc;
    RefreshRequestFunc mRefreshRequestFunc;
    HandleResponseFunc mHandleResponseFunc;
    AtomicLong mLastRefreshTime;
    Mutex mMtx;
    String mPassword;
    int mMasterReadPriority;
    int mStaticSlaveReadPriority;
    int mDynamicSlaveReadPriority;
    long mRefreshInterval;
    long mServerTimeout;
    int mServerFailureLimit;
    long mServerRetryTimeout;
    int mKeepAlive;
    int mDbNum;
};

template<class T>
class ServerPoolTmpl : public ServerPool
{
public:
    ServerPoolTmpl(Proxy* p, int type):
        ServerPool(p, type, (ServerPoolTmpl<T>*)this)
    {
    }
private:
    static Server* getServer(const ServerPool* p, Handler* h, Request* req, const String& key)
    {
        return static_cast<const T*>(p)->getServer(h, req, key);
    }
    static Server* iter(const ServerPool* p, int& cursor)
    {
        return static_cast<const T*>(p)->iter(cursor);
    }
    static void refreshRequest(ServerPool* p, Handler* h)
    {
        static_cast<T*>(p)->refreshRequest(h);
    }
    static void handleResponse(ServerPool* p, Handler* h, ConnectConnection* s, Request* req, Response* res)
    {
        static_cast<T*>(p)->handleResponse(h, s, req, res);
    }
    friend class ServerPool;
};

#endif
