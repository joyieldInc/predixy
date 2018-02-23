/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Server.h"
#include "ServerPool.h"
#include "Handler.h"

ServerPool::~ServerPool()
{
}

void ServerPool::init(const ServerPoolConf& conf)
{
    mPassword = conf.password;
    mMasterReadPriority = conf.masterReadPriority;
    mStaticSlaveReadPriority = conf.staticSlaveReadPriority;
    mDynamicSlaveReadPriority = conf.dynamicSlaveReadPriority;
    mRefreshInterval = conf.refreshInterval;
    mServerTimeout = conf.serverTimeout;
    mServerFailureLimit = conf.serverFailureLimit;
    mServerRetryTimeout = conf.serverRetryTimeout;
    mKeepAlive = conf.keepalive;
    mDbNum = conf.databases;
}

bool ServerPool::refresh()
{
    long last = mLastRefreshTime;
    long now = Util::nowUSec();
    if (now - last < mRefreshInterval) {
        return false;
    }
    return AtomicCAS(mLastRefreshTime, last, now);
}

void ServerPool::handleResponse(Handler* h, ConnectConnection* s, Request* req, Response* res)
{
    UniqueLock<Mutex> lck(mMtx, TryLockTag);
    if (!lck) {
        logNotice("server pool is updating by other thread");
        return;
    }
    mHandleResponseFunc(this, h, s, req, res);
}

Server* ServerPool::randServer(Handler* h, const std::vector<Server*>& servs)
{
    Server* s = nullptr;
    int idx = h->rand() % servs.size();
    for (size_t i = 0; i < servs.size(); ++i) {
        Server* serv = servs[idx++ % servs.size()];
        if (!serv->online()) {
            continue;
        } else if (serv->fail()) {
            s = serv;
        } else {
            return serv;
        }
    }
    return s;
}

Server* ServerPool::iter(const std::vector<Server*>& servs, int& cursor)
{
    int size = servs.size();
    if (cursor < 0 || cursor >= size) {
        return nullptr;
    }
    while (cursor < size) {
        Server* serv = servs[cursor++];
        if (serv->online()) {
            return serv;
        }
    }
    return nullptr;
}
