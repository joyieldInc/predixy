/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SENTINEL_SERVER_POOL_H_
#define _PREDIXY_SENTINEL_SERVER_POOL_H_

#include <map>
#include "Predixy.h"
#include "ServerPool.h"

class SentinelServerPool : public ServerPoolTmpl<SentinelServerPool>
{
public:
    static const int MaxSentinelNum = 64;
public:
    SentinelServerPool(Proxy* p);
    ~SentinelServerPool();
    void init(const SentinelServerPoolConf& conf);
    Server* getServer(Handler* h, Request* req, const String& key) const;
    Server* iter(int& cursor) const
    {
        return ServerPool::iter(mServPool, cursor);
    }
    void refreshRequest(Handler* h);
    void handleResponse(Handler* h, ConnectConnection* s, Request* req, Response* res);
private:
    void handleSentinels(Handler* h, ConnectConnection* s, Request* req, Response* res);
    void handleGetMaster(Handler* h, ConnectConnection* s, Request* req, Response* res);
    void handleSlaves(Handler* h, ConnectConnection* s, Request* req, Response* res);
    friend class ServerPoolTmpl<SentinelServerPool>;
private:
    std::vector<Server*> mSentinels;
    std::vector<Server*> mServPool;
    Distribution mDist;
    Hash mHash;
    char mHashTag[2];
};

#endif
