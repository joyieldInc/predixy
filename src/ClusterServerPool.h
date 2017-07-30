/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CLUSTER_SERVER_POOL_H_
#define _PREDIXY_CLUSTER_SERVER_POOL_H_

#include <vector>
#include <string>
#include <map>
#include "ServerPool.h"

class ClusterServerPool : public ServerPoolTmpl<ClusterServerPool>
{
public:
    static const char* HashTag;
public:
    ClusterServerPool(Proxy* p);
    ~ClusterServerPool();
    void init(const ClusterServerPoolConf& conf);
    Server* redirect(const String& addr, Server* old) const;
    const std::vector<Server*>& servers() const
    {
        return mServPool;
    }
private:
    Server* getServer(Handler* h, Request* req, const String& key) const;
    void refreshRequest(Handler* h);
    void handleResponse(Handler* h, ConnectConnection* s, Request* req, Response* res);
    ServerGroup* getGroup(const String& nodeid) const
    {
        auto it = mGroups.find(nodeid);
        return it == mGroups.end() ? nullptr : it->second;
    }
    Server* iter(int& cursor) const
    {
        return ServerPool::iter(mServPool, cursor);
    }
    friend class ServerPoolTmpl<ClusterServerPool>;
private:
    Hash mHash;
    std::vector<Server*> mServPool;
    std::map<String, ServerGroup*> mGroups;
    ServerGroup* mSlots[Const::RedisClusterSlots];
};

#endif
