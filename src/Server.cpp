/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Proxy.h"
#include "Server.h"
#include "ServerPool.h"

const char* Server::RoleStr[] = {
    "unknown",
    "master",
    "slave",
    "sentinel"
};

Server::Server(ServerPool* pool, const String& addr, bool isStatic):
    mPool(pool),
    mGroup(nullptr),
    mRole(Unknown),
    mDC(nullptr),
    mAddr(addr),
    mNextActivateTime(0),
    mFailureCnt(0),
    mStatic(isStatic),
    mFail(false),
    mOnline(true),
    mUpdating(false)
{
    if (auto dataCenter = pool->proxy()->dataCenter()) {
        mDC = dataCenter->getDC(addr);
    }
}

Server::~Server()
{
}

bool Server::activate()
{
    long v = mNextActivateTime;
    long now = Util::nowUSec();
    if (now < v) {
        return false;
    }
    return AtomicCAS(mNextActivateTime, v, now + mPool->serverRetryTimeout());
}

// increase the fail counter of the server
// and set status if the fail counter already reach the threshold
void Server::incrFail()
{
    long cnt = ++mFailureCnt;
    if (cnt % mPool->serverFailureLimit() == 0) {
        logError("[ibk]SET FAIL TRUE");
        setFail(true);
    }
}

