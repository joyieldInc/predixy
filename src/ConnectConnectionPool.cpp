/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Proxy.h"
#include "ConnectConnectionPool.h"

ConnectConnectionPool::ConnectConnectionPool(Handler* h, Server* s, int dbnum):
    mHandler(h),
    mServ(s),
    mPendRequests(0),
    mShareConns(dbnum),
    mPrivateConns(dbnum),
    mLatencyMonitors(h->latencyMonitors())
{
    resetStats();
}

ConnectConnectionPool::~ConnectConnectionPool()
{
}

ConnectConnection* ConnectConnectionPool::getShareConnection(int db)
{
    FuncCallTimer();
    if (db >= (int)mShareConns.size()) {
        logWarn("h %d get share connection for db %d >= %d",
                mHandler->id(), db, (int)mShareConns.size());
        return nullptr;
    }
    bool needInit = false;
    ConnectConnection* c = mShareConns[db];
    if (!c) {
        c = ConnectConnectionAlloc::create(mServ, true);
        c->setDb(db);
        ++mStats.connections;
        mShareConns[db] = c;
        needInit = true;
        logNotice("h %d create server connection %s %d",
                  mHandler->id(), c->peer(), c->fd());
    } else if (c->fd() < 0) {
        if (mServ->fail()) {
            return nullptr;
        }
        c->reopen();
        needInit = true;
        logNotice("h %d reopen server connection %s %d",
                  mHandler->id(), c->peer(), c->fd());
    }
    if (needInit && !init(c)) {
        c->close(mHandler);
        return nullptr;
    }
    if (mServ->fail()) {
        return nullptr;
    }
    return c;
}

// func ini entah dipanggil oleh siapa
ConnectConnection* ConnectConnectionPool::getPrivateConnection(int db)
{
    FuncCallTimer();
    if (db >= (int)mPrivateConns.size()) {
        logWarn("h %d get private connection for db %d >= %d",
                mHandler->id(), db, (int)mPrivateConns.size());
        return nullptr;
    }
    auto& ccl = mPrivateConns[db];
    ConnectConnection* c = ccl.pop_front();
    bool needInit = false;
    if (!c) {
        if (mServ->fail()) {
            return nullptr;
        }
        c = ConnectConnectionAlloc::create(mServ, false);
        c->setDb(db);
        ++mStats.connections;
        needInit = true;
        logNotice("h %d create private server connection %s %d",
                  mHandler->id(), c->peer(), c->fd());
    }
    if (c->fd() < 0) {
        if (mServ->fail()) {
            return nullptr;
        }
        c->reopen();
        needInit = true;
        logNotice("h %d reopen server connection %s %d",
                  mHandler->id(), c->peer(), c->fd());
    }
    if (needInit && !init(c)) {
        c->close(mHandler);
        ccl.push_back(c);
        return nullptr;
    }
    return mServ->fail() ? nullptr : c;
}

void ConnectConnectionPool::putPrivateConnection(ConnectConnection* s)
{
    logDebug("h %d put private connection s %s %d",
            mHandler->id(), s->peer(), s->fd());
    unsigned db = s->db();
    if (db < mPrivateConns.size()) {
        mPrivateConns[db].push_back(s);
    } else {
        logWarn("h %d s %s %d put to pool with db %d invalid",
                mHandler->id(), s->peer(), s->fd(), s->db());
    }
}

bool ConnectConnectionPool::init(ConnectConnection* c)
{
    if (!c->setNonBlock()) {
        logWarn("h %d s %s %d set non block fail",
                mHandler->id(), c->peer(), c->fd());
        return false;
    }
    if (!c->setTcpNoDelay()) {
        logWarn("h %d s %s %d settcpnodelay fail %s",
                mHandler->id(), c->peer(), c->fd(), StrError());
    }
    auto sp = mHandler->proxy()->serverPool();
    if (sp->keepalive() > 0 && !c->setTcpKeepAlive(sp->keepalive())) {
        logWarn("h %d s %s %d settcpkeepalive(%d) fail %s",
                mHandler->id(), c->peer(), c->fd(), sp->keepalive(),StrError());
    }
    auto m = mHandler->eventLoop();
    if (!m->addSocket(c, Multiplexor::ReadEvent|Multiplexor::WriteEvent)) {
        logWarn("h %d s %s %d add to eventloop fail",
                mHandler->id(), c->peer(), c->fd());
        return false;
    }
    ++mStats.connect;
    if (!c->connect()) {
        logWarn("h %d s %s %d connect fail",
                mHandler->id(), c->peer(), c->fd());
        m->delSocket(c);
        return false;
    }
    if (mServ->password().empty()) {
        c->setAuth(true);
    } else {
        c->setAuth(false);
        RequestPtr req = RequestAlloc::create();
        req->setAuth(mServ->password());
        mHandler->handleRequest(req, c);
        logDebug("h %d s %s %d auth req %ld",
                mHandler->id(), c->peer(), c->fd(), req->id());
    }
    if (sp->type() == ServerPool::Cluster) {
        RequestPtr req = RequestAlloc::create(Request::Readonly);
        mHandler->handleRequest(req, c);
        logDebug("h %d s %s %d readonly req %ld",
                mHandler->id(), c->peer(), c->fd(), req->id());
    }
    int db = c->db();
    if (db != 0) {
        RequestPtr req = RequestAlloc::create();
        req->setSelect(db);
        mHandler->handleRequest(req, c);
        logDebug("h %d s %s %d select %d req %ld",
                mHandler->id(), c->peer(), c->fd(), db, req->id());
    }
    RequestPtr req = RequestAlloc::create(Request::PingServ);
    mHandler->handleRequest(req, c);
    logDebug("h %d s %s %d ping req %ld",
             mHandler->id(), c->peer(), c->fd(), req->id());
    return true;
}

void ConnectConnectionPool::check()
{
    FuncCallTimer();
    if (!mServ->fail() || !mServ->online()) {
        return;
    }
    if (mServ->activate()) {
        auto c = mShareConns.empty() ? nullptr : mShareConns[0];
        if (!c) {
            return;
        }
        if (c->fd() >= 0) {
            return;
        }
        c->reopen();
        logNotice("h %d check server reopen connection %s %d",
                  mHandler->id(), c->peer(), c->fd());
        if (!init(c)) {
            c->close(mHandler);
        }
    }
}
