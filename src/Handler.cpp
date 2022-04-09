/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <sys/resource.h>
#include <iostream>
#include "Alloc.h"
#include "Handler.h"
#include "Proxy.h"
#include "ListenSocket.h"
#include "ServerGroup.h"
#include "AcceptConnection.h"
#include "ConnectConnection.h"
#include "ClusterServerPool.h"
#include "SentinelServerPool.h"

Handler::Handler(Proxy* p):
    mStop(false),
    mProxy(p),
    mEventLoop(new Multiplexor()),
    mStatsVer(0),
    mLatencyMonitors(p->latencyMonitorSet().latencyMonitors()),
    mRandSeed(time(nullptr) * (id() + 1))
{
    if (!mEventLoop->addSocket(p->listener())) {
        logError("handler %d add listener to multiplexor fail:%sa",
                 id(), StrError());
        Throw(AddListenerEventFail, "handler %d add listener to multiplexor fail:%sa",
                 id(), StrError());
    }
    mConnPool.reserve(Const::MaxServNum);
    Conf* conf = p->conf();
    mIDUnique.resize(conf->dcConfs().size());
}

Handler::~Handler()
{
}

void Handler::run()
{
    Request::init();
    Response::init();
    auto conf = mProxy->conf();
    refreshServerPool();
    while (!mStop) {
        mEventLoop->wait(100000, this);
        postEvent();
        long timeout = conf->clientTimeout();
        if (timeout > 0) {
            int num = checkClientTimeout(timeout);
            if (num > 0) {
                postEvent();
            }
        }
        refreshServerPool();
        checkConnectionPool();
        timeout = mProxy->serverPool()->serverTimeout();
        if (timeout > 0) {
            int num = checkServerTimeout(timeout);
            if (num > 0) {
                postEvent();
            }
        }
        if (mStatsVer < mProxy->statsVer()) {
            resetStats();
        }
    }
    logNotice("handler %d stopped", id());
}

void Handler::stop()
{
    mStop = true;
}

void Handler::refreshServerPool()
{
    FuncCallTimer();
    try {
        ServerPool* sp = mProxy->serverPool();
        if (!sp->refresh()) {
            return;
        }
        sp->refreshRequest(this);
    } catch (ExceptionBase& excp) {
        logError("h %d refresh server pool exception:%s",
                id(), excp.what());
    }
}

void Handler::checkConnectionPool()
{
    for (auto p : mConnPool) {
        try {
            if (p) {
                p->check();
            }
        } catch (ExceptionBase& excp) {
            logError("h %d check connection pool %s excp %s",
                      id(), p->server()->addr().data(), excp.what());
        }
    }
}

int Handler::checkServerTimeout(long timeout)
{
    int num = 0;
    auto now = Util::elapsedUSec();
    auto n = mWaitConnectConns.front();
    while (n) {
        auto s = n;
        n = mWaitConnectConns.next(n);
        if (auto req = s->frontRequest()) {
            long elapsed = now - req->createTime();
            if (elapsed >= timeout) {
                logError("ibk: SET_STATUS TIMEOUT ERROR: %d",Connection::TimeoutError);
                s->setStatus(Connection::TimeoutError);
                addPostEvent(s, Multiplexor::ErrorEvent);
                mWaitConnectConns.remove(s);
                ++num;
            }
        } else {
            mWaitConnectConns.remove(s);
        }
    }
    return num;
}

void Handler::handleEvent(Socket* s, int evts)
{
    FuncCallTimer();
    switch (s->classType()) {
    case Socket::ListenType:
        handleListenEvent(static_cast<ListenSocket*>(s), evts);
        break;
    case Connection::AcceptType:
        handleAcceptConnectionEvent(static_cast<AcceptConnection*>(s),evts);
        break;
    case Connection::ConnectType:
        handleConnectConnectionEvent(static_cast<ConnectConnection*>(s),evts);
        break;
    default:
        //should no reach
        logError("h %d unexpect socket %d ev %d", id(), s->fd(), evts);
        break;
    }
}

void Handler::postEvent()
{
    FuncCallTimer();
    while (!mPostAcceptConns.empty() || !mPostConnectConns.empty()) {
        if (!mPostConnectConns.empty()) {
            postConnectConnectionEvent();
        }
        if (!mPostAcceptConns.empty()) {
            postAcceptConnectionEvent();
        }
    }
}

inline void Handler::addPostEvent(AcceptConnection* c, int evts)
{
    if (!c->getPostEvent()) {
        mPostAcceptConns.push_back(c);
        setAcceptConnectionActiveTime(c);
    }
    c->addPostEvent(evts);
}

inline void Handler::addPostEvent(ConnectConnection* s, int evts)
{
    if (!s->getPostEvent()) {
        mPostConnectConns.push_back(s);
    }
    s->addPostEvent(evts);
}

void Handler::postAcceptConnectionEvent()
{
    FuncCallTimer();
    while (!mPostAcceptConns.empty()) {
        AcceptConnection* c = mPostAcceptConns.front();
        int evts = c->getPostEvent();
        c->setPostEvent(0);
        if (c->good() && (evts & Multiplexor::WriteEvent)) {
            bool finished = c->writeEvent(this);
            if (c->good()) {
                bool ret;
                if (finished) {
                    ret = mEventLoop->delEvent(c, Multiplexor::WriteEvent);
                    if (c->isCloseASAP()) {
                        c->setStatus(AcceptConnection::None);
                    }
                } else {
                    ret = mEventLoop->addEvent(c, Multiplexor::WriteEvent);
                }
                if (!ret) {
                    c->setStatus(AcceptConnection::IOError);
                }
            }
        }
        if ((!c->good()||(evts & Multiplexor::ErrorEvent)) && c->fd() >= 0){
            logNotice("h %d remove c %s %d with status %d %s",
                    id(), c->peer(), c->fd(), c->status(), c->statusStr());
            mEventLoop->delSocket(c);
            if (auto s = c->connectConnection()) {
                auto cp = mConnPool[s->server()->id()];
                s->setStatus(Connection::LogicError);
                addPostEvent(s, Multiplexor::ErrorEvent);
                c->detachConnectConnection();
                s->detachAcceptConnection();
            }
            mAcceptConns.remove(c);
            c->unref();
            c->close();
            --mStats.clientConnections;
        }
        mPostAcceptConns.pop_front();
    }
}

void Handler::postConnectConnectionEvent()
{
    FuncCallTimer();
    while (!mPostConnectConns.empty()) {
        ConnectConnection* s = mPostConnectConns.pop_front();
        int evts = s->getPostEvent();
        s->setPostEvent(0);
        if (s->good() && evts == Multiplexor::WriteEvent && !s->isConnecting()) {
            bool finished = s->writeEvent(this);
            if (s->good()) {
                bool ret;
                if (finished) {
                    ret = mEventLoop->delEvent(s, Multiplexor::WriteEvent);
                } else {
                    ret = mEventLoop->addEvent(s, Multiplexor::WriteEvent); // ibk: add to the epoll
                }
                if (!ret) {
                    s->setStatus(Multiplexor::ErrorEvent);
                } else {
                    if (s->isShared() && !mWaitConnectConns.exist(s)) {
                        mWaitConnectConns.push_back(s);
                    }
                }
            }
        }
        if ((!s->good()||(evts & Multiplexor::ErrorEvent)) && s->fd() >= 0){
            switch (s->status()) {
            case Socket::End:
            case Socket::IOError:
            case Socket::EventError:
                {
                    Server* serv = s->server();
                    serv->incrFail(); // ibk:increase fail counter only for EventError, mark as failed if exceed threshold
                    if (serv->fail()) {
                        logNotice("server %s mark failure", serv->addr().data());
                    }
                }
                break;
            default:
                break;
            }
            auto c = s->acceptConnection();
            logNotice("h %d close s %s %d and c %s %d with status %d %s",
                    id(), s->peer(), s->fd(),
                    c ? c->peer() : "None", c ? c->fd() : -1,
                    s->status(), s->statusStr());
            mEventLoop->delSocket(s);
            s->close(this);
            if (!s->isShared()) {
                mConnPool[s->server()->id()]->putPrivateConnection(s);
            }
            if (c) {
                addPostEvent(c, Multiplexor::ErrorEvent);
                s->detachAcceptConnection();
                c->detachConnectConnection();
            }
        }
    }
}

void Handler::handleListenEvent(ListenSocket* s, int evts)
{
    FuncCallTimer();
    while (true) {
        try {
            sockaddr_storage addr;
            socklen_t len = sizeof(addr);
            int fd = s->accept((sockaddr*)&addr, &len);
            if (fd >= 0) {
                ++mStats.accept;
                addAcceptSocket(fd, (sockaddr*)&addr, len);
            } else {
                break;
            }
        } catch (ListenSocket::TooManyOpenFiles& f) {
            logNotice("h %d p %s %d e %s", id(), s->addr(), s->fd(), f.what());
            break;
        } catch (ExceptionBase& e) {
            logError("h %d p %s %d e %s", id(), s->addr(), s->fd(), e.what());
            throw;
        }
    }
}

void Handler::addAcceptSocket(int fd, sockaddr* addr, socklen_t len)
{
    FuncCallTimer();
    AcceptConnection* c = nullptr;
    try {
        c = AcceptConnectionAlloc::create(fd, addr, len);
    } catch (ExceptionBase& e) {
        logWarn("h %d create connection for client %d fail %s",
                id(), fd, e.what());
        ::close(fd);
        return;
    }
    if (!c->setNonBlock()) {
        logWarn("h %d destroy c %s %d with setnonblock fail:%s",
                id(), c->peer(), c->fd(), StrError());
        AcceptConnectionAlloc::destroy(c);
        return;
    }
    if (addr->sa_family == AF_INET || addr->sa_family == AF_INET6) {
        if (!c->setTcpNoDelay()) {
            logNotice("h %d ignore c %s %d settcpnodelay fail:%s",
                    id(), c->peer(), c->fd(), StrError());
        }
    }
    if (auto auth = mProxy->authority()->getDefault()) {
        c->setAuth(auth);
    }
    Handler* dst = this;
#ifdef _MULTIPLEXOR_ASYNC_ASSIGN_
    int clients = INT_MAX;
    for (auto h : mProxy->handlers()) {
        int num = h->mAcceptConns.size();
        if (num < clients) {
            clients = num;
            dst = h;
        }
    }
#endif
    c->ref();
    bool fail = false;
    if (dst == this) {
        if (mEventLoop->addSocket(c)) {
            c->setLastActiveTime(Util::elapsedUSec());
            mAcceptConns.push_back(c);
            ++mStats.clientConnections;
        } else {
            fail = true;
        }
    } else {
        c->setLastActiveTime(-1);
        if (!dst->mEventLoop->addSocket(c, Multiplexor::ReadEvent|Multiplexor::WriteEvent)) {
            fail = true;
        }
    }
    if (fail) {
        logWarn("h %d destroy c %s %d with add to event loop fail:%s",
                id(), c->peer(), c->fd(), StrError());
        AcceptConnectionAlloc::destroy(c);
    } else {
        logNotice("h %d accept c %s %d assign to h %d", id(), c->peer(), fd, dst->id());
    }
}

void Handler::handleAcceptConnectionEvent(AcceptConnection* c, int evts)
{
    FuncCallTimer();
    logVerb("h %d c %s %d ev %d", id(), c->peer(), c->fd(), evts);
    if (c->lastActiveTime() < 0) {
        c->setLastActiveTime(Util::elapsedUSec());
        mAcceptConns.push_back(c);
        ++mStats.clientConnections;
    }
    if (evts & Multiplexor::ErrorEvent) {
        c->setStatus(AcceptConnection::EventError);
    }
    try {
        if (c->good() && (evts & Multiplexor::ReadEvent)) {
            c->readEvent(this);
        }
        if (c->good() && (evts & Multiplexor::WriteEvent)) {
            addPostEvent(c, Multiplexor::WriteEvent);
        }
    } catch (ExceptionBase& e) {
        logWarn("h %d c %s %d handle event %d exception %s",
                id(), c->peer(), c->fd(), evts, e.what());
        c->setStatus(AcceptConnection::ExceptError);
    }
    if (!c->good()) {
        addPostEvent(c, Multiplexor::ErrorEvent);
        logDebug("h %d c %s %d will be close with status %d %s",
                id(), c->peer(), c->fd(), c->status(), c->statusStr());
    }
}

void Handler::handleConnectConnectionEvent(ConnectConnection* s, int evts)
{
    FuncCallTimer();
    logVerb("h %d s %s %d ev %d", id(), s->peer(), s->fd(), evts);
    if (evts & Multiplexor::ErrorEvent) {
        logDebug("h %d s %s %d error event",
                id(), s->peer(), s->fd());
        s->setStatus(ConnectConnection::EventError);
    }
    try {
        if (s->good() && (evts & Multiplexor::ReadEvent)) {
            s->readEvent(this);
        }
        if (s->good() && (evts & Multiplexor::WriteEvent)) {
            if (s->isConnecting()) {
                s->setConnected();
                logDebug("h %d s %s %d connected",
                        id(), s->peer(), s->fd());
            }
            addPostEvent(s, Multiplexor::WriteEvent);
        }
    } catch (ExceptionBase& e) {
        logError("h %d s %s %d handle event %d exception %s",
                id(), s->peer(), s->fd(), evts, e.what());
        s->setStatus(ConnectConnection::ExceptError);
    }
    if (!s->good()) {
        addPostEvent(s, Multiplexor::ErrorEvent);
        logError("h %d s %s %d will be close with status %d %s",
                id(), s->peer(), s->fd(), s->status(), s->statusStr());
    }
}

ConnectConnection* Handler::getConnectConnection(Request* req, Server* serv)
{
    FuncCallTimer();
    unsigned sid = serv->id();
    auto p = sid < mConnPool.size() ? mConnPool[sid] : nullptr;
    if (!p) {
        if (sid >= mConnPool.size()) {
            if (sid >= mConnPool.capacity()) {
                logError("h %d too many servers %d in server pool ignore server %s",
                        id(), sid, serv->addr().data());
                return nullptr;
            }
            mConnPool.resize(sid + 1, nullptr);
        }
        logNotice("h %d create connection pool for server %s",
                id(), serv->addr().data());
        p = new ConnectConnectionPool(this, serv, serv->pool()->dbNum());
        mConnPool[sid] = p;
    }
    p->stats().requests++;
    int db = 0;
    auto c = req->connection();
    if (c) {
        if (auto s = c->connectConnection()) {
            return s;
        }
        db = c->db();
    }
    if (req->requirePrivateConnection()) {
        return p->getPrivateConnection(db);
    }
    return p->getShareConnection(db);
}

int Handler::checkClientTimeout(long timeout)
{
    int num = 0;
    long now = Util::elapsedUSec();
    auto c = mAcceptConns.front();
    while (c) {
        long t = now - c->lastActiveTime();
        if (t < timeout) {
            break;
        }
        if (c->empty()) {
            ++num;
            addPostEvent(c, Multiplexor::ErrorEvent);
            logDebug("h %d c %s %d timeout",
                    id(), c->peer(), c->fd());
            c = mAcceptConns.next(c);
        } else {
            auto n = mAcceptConns.next(c);
            c->setLastActiveTime(now);
            mAcceptConns.move_back(c);
            c = n;
        }
    }
    return num;
}

// when entering this function, predixy already split multi keys command into multiple commands
// e.g. client send : mget a b
// this func will receive `mget a` and `mget b` 
void Handler::handleRequest(Request* req)
{
    FuncCallTimer();
    auto c = req->connection();
    if (c && (c->isBlockRequest() || c->isCloseASAP())) {
        return;
    }
    ++mStats.requests;
    req->setDelivered();
    SegmentStr<Const::MaxKeyLen> key(req->key());
    logDebug("h %d c %s %d handle req %ld %s %.*s",
            id(), c ? c->peer() : "None", c ? c->fd() : -1,
            req->id(), req->cmd(), key.length(), key.data());
    Response::GenericCode code;
    if (!permission(req, key, code)) {
        directResponse(req, code);
        return;
    }
    if (preHandleRequest(req, key)) {
        return;
    }
    auto sp = mProxy->serverPool();
    Server* serv = sp->getServer(this, req, key);
    if (!serv) {
        directResponse(req, Response::NoServer);
        return;
    }
    ConnectConnection* s = getConnectConnection(req, serv);
    if (!s || !s->good()) {
        directResponse(req, Response::NoServerConnection);
        return;
    }
    if (s->isShared()) {
        mConnPool[serv->id()]->incrPendRequests();
    }
    // ibk: these three lines only add the event in the corresponding queues
    s->send(this, req);
    addPostEvent(s, Multiplexor::WriteEvent);
    postHandleRequest(req, s);
}

bool Handler::preHandleRequest(Request* req, const String& key)
{
    FuncCallTimer();
    auto c = req->connection();
    if (c && c->inTransaction()) {
        switch (req->type()) {
        case Command::Select:
        case Command::Psubscribe:
        case Command::Subscribe:
            {
                ResponsePtr res = ResponseAlloc::create();
                char buf[128];
                snprintf(buf, sizeof(buf), "forbid command \"%s\" in transaction",
                        req->cmd());
                res->setErr(buf);
                handleResponse(nullptr, req, res);
                addPostEvent(c, Multiplexor::ErrorEvent);
                return true;
            }
        default:
            break;
        }
        return false;
    }
    switch (req->type()) {
    case Command::Ping:
    case Command::Echo:
        if (key.empty()) {
            directResponse(req, Response::Pong);
        } else {
            ResponsePtr res = ResponseAlloc::create();
            if (req->isInline()) {
                SString<Const::MaxKeyLen> k;
                RequestParser::decodeInlineArg(k, key);
                res->setStr(k.data(), k.length());
            } else {
                res->setStr(key.data(), key.length());
            }
            handleResponse(nullptr, req, res);
        }
        return true;
    case Command::Select:
        {
            int db = -1;
            if (key.toInt(db)) {
                if (db >= 0 && db < mProxy->serverPool()->dbNum()) {
                    c->setDb(db);
                    directResponse(req, Response::Ok);
                    return true;
                }
            }
            directResponse(req, Response::InvalidDb);
        }
        return true;
    case Command::Quit:
        directResponse(req, Response::Ok);
        if (c) {
            c->closeASAP();
        }
        return true;
    case Command::Cmd:
        directResponse(req, Response::Cmd);
        return true;
    case Command::Info:
        infoRequest(req, key);
        return true;
    case Command::Config:
        configRequest(req, key);
        return true;
    case Command::Script:
        if (key.length() == 4 && strncasecmp(key.data(), "load", 4) == 0) {
            int cursor = 0;
            auto sp = mProxy->serverPool();
            Server* leaderServ = sp->iter(cursor);
            if (!leaderServ) {
                directResponse(req, Response::NoServer);
                return true;
            }
            req->setType(Command::ScriptLoad);
            req->follow(req);
            while (Server* serv = sp->iter(cursor)) {
                RequestPtr r = RequestAlloc::create();
                r->follow(req);
                ConnectConnection* s = getConnectConnection(r, serv);
                if (!s) {
                    directResponse(r, Response::NoServerConnection);
                    break;
                }
                handleRequest(r, s);
            }
            //req must be handle in the last, avoid directResponse this req
            ConnectConnection* s = getConnectConnection(req, leaderServ);
            if (!s) {
                directResponse(req, Response::NoServerConnection);
                return true;
            }
            handleRequest(req, s);
        } else {
            directResponse(req, Response::UnknownCmd);
        }
        return true;
    case Command::Watch:
    case Command::Multi:
        if (!mProxy->supportTransaction()) {
            directResponse(req, Response::ForbidTransaction);
            return true;
        }
        break;
    case Command::Scan:
        {
            auto sp = mProxy->serverPool();
            unsigned long cursor = atol(key.data());
            int groupIdx = cursor & Const::ServGroupMask;
            auto g = sp->getGroup(groupIdx);
            if (!g) {
                directResponse(req, Response::InvalidScanCursor);
                return true;
            }
            Server* serv = g->getMaster();
            while (!serv && (g = sp->getGroup(++groupIdx)) != nullptr) {
                serv = g->getMaster();
            }
            if (!serv) {
                directResponse(req, Response::ScanEnd);
                return true;
            }
            if (ConnectConnection* s = getConnectConnection(req, serv)) {
                if (cursor != 0) {
                    req->adjustScanCursor(cursor >> Const::ServGroupBits);
                }
                handleRequest(req, s);
            } else {
                directResponse(req, Response::NoServerConnection);
            }
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

void Handler::postHandleRequest(Request* req, ConnectConnection* s)
{
    FuncCallTimer();
    auto c = req->connection();
    if (!c) {
        return;
    }
    switch (req->type()) {
    case Command::Blpop:
    case Command::Brpop:
    case Command::Brpoplpush:
        c->setBlockRequest(true);
        c->attachConnectConnection(s);
        s->attachAcceptConnection(c);
        break;
    case Command::Unwatch:
    case Command::Exec:
    case Command::Discard:
    case Command::Punsubscribe:
    case Command::Unsubscribe:
        c->setBlockRequest(true);
        break;
    case Command::Watch:
        if (c->incrPendWatch() <= 0) {
            addPostEvent(c, Multiplexor::ErrorEvent);
            return;
        }
        c->attachConnectConnection(s);
        s->attachAcceptConnection(c);
        break;
    case Command::Multi:
        if (c->incrPendMulti() <= 0) {
            addPostEvent(c, Multiplexor::ErrorEvent);
            return;
        }
        c->attachConnectConnection(s);
        s->attachAcceptConnection(c);
        break;
    case Command::Psubscribe:
    case Command::Subscribe:
        if (c->incrPendSub() <= 0) {
            addPostEvent(c, Multiplexor::ErrorEvent);
            return;
        }
        c->attachConnectConnection(s);
        s->attachAcceptConnection(c);
        break;
    default:
        break;
    }
}

void Handler::handleRequest(Request* req, ConnectConnection* s)
{
    if (!s->good()) {
        return;
    }
    s->send(this, req);
    addPostEvent(s, Multiplexor::WriteEvent);
    mStats.requests++;
    mConnPool[s->server()->id()]->stats().requests++;
    if (s->isShared()) {
        mConnPool[s->server()->id()]->incrPendRequests();
    }
}

void Handler::directResponse(Request* req, Response::GenericCode code, ConnectConnection* s)
{
    FuncCallTimer();
    if (auto c = req->connection()) {
        if (c->good()) {
            try {
                ResponsePtr res = ResponseAlloc::create(code);
                handleResponse(s, req, res);
            } catch (ExceptionBase& excp) {
                c->setStatus(AcceptConnection::LogicError);
                addPostEvent(c, Multiplexor::ErrorEvent);
                logWarn("h %d c %s %d will be close req %ld direct response %d excp %s",
                        id(), c->peer(), c->fd(), req->id(), code, excp.what());
            }
        } else {
            logDebug("h %d ignore req %ld res code %d c %s %d status %d %s",
                    id(), req->id(), code, c->peer(), c->fd(), c->status(), c->statusStr());
        }
    } else {
        logDebug("h %d ignore req %ld res code %d without accept connection",
                id(), req->id(), code);
    }
}

void Handler::handleResponse(ConnectConnection* s, Request* req, Response* res)
{
    FuncCallTimer();
    SegmentStr<Const::MaxKeyLen> key(req->key());
    logDebug("h %d s %s %d req %ld %s %.*s res %ld %s",
            id(), (s ? s->peer() : "None"), (s ? s->fd() : -1),
            req->id(), req->cmd(), key.length(), key.data(),
            res->id(), res->typeStr());
    mStats.responses++;
    if (s) {
        mConnPool[s->server()->id()]->stats().responses++;
        if (s->isShared()) {
            mConnPool[s->server()->id()]->decrPendRequests();
        }
    }
    if (req->isInner()) {
        innerResponse(s, req, res);
        return;
    }
    auto sp = mProxy->serverPool();
    AcceptConnection* c = req->connection();
    if (!c) {
        logDebug("h %d ignore req %ld res %ld", id(), req->id(), res->id());
        return;
    } else if (!c->good()) {
        logWarn("h %d ignore req %ld res %ld for c %s %d with status %d %s",
                id(), req->id(), res->id(),
                c->peer(), c->fd(), c->status(), c->statusStr());
        return;
    }
    if (sp->type() == ServerPool::Cluster && res->type() == Reply::Error) {
        if (res->isMoved()) {
            if (redirect(s, req, res, true)) {
                return;
            }
        } else if (res->isAsk()) {
            if (redirect(s, req, res, false)) {
                return;
            }
        }
    } else if (req->type() == Command::Scan && s && res->type() == Reply::Array) {
        SegmentStr<64> str(res->body());
        if (const char* p = strchr(str.data() + sizeof("*2\r\n$"), '\n')) {
            long cursor = atol(p + 1);
            auto g = s->server()->group();
            if (cursor != 0 || (g = sp->getGroup(g->id() + 1)) != nullptr) {
                cursor <<= Const::ServGroupBits;
                cursor |= g->id();
                if ((p = strchr(p, '*')) != nullptr) {
                    char buf[32];
                    int n = snprintf(buf, sizeof(buf), "%ld", cursor);
                    res->head().fset(nullptr,
                            "*2\r\n"
                            "$%d\r\n"
                            "%s\r\n",
                            n, buf);
                    res->body().cut(p - str.data());
                }
            }
        }
    }
    if (req->leader()) {
        res->adjustForLeader(req);
    }
    req->setResponse(res);
    if (c->send(this, req, res)) {
        addPostEvent(c, Multiplexor::WriteEvent);
    }
    long elapsed = Util::elapsedUSec() - req->createTime();
    if (auto cp = s ? mConnPool[s->server()->id()] : nullptr) {
        for (auto i : mProxy->latencyMonitorSet().cmdIndex(req->type())) {
            int idx = mLatencyMonitors[i].add(elapsed);
            if (idx >= 0) {
                cp->latencyMonitors()[i].add(elapsed, idx);
            }
        }
    } else {
        for (auto i : mProxy->latencyMonitorSet().cmdIndex(req->type())) {
            mLatencyMonitors[i].add(elapsed);
        }
    }
    logInfo("RESP h %d c %s %d req %ld %s %.*s s %s %d res %ld %s t %ld",
            id(), c->peer(), c->fd(),
            req->id(), req->cmd(), key.length(), key.data(),
            (s ? s->peer() : "None"), (s ? s->fd() : -1),
            res->id(), res->typeStr(), elapsed);
    switch (req->type()) {
    case Command::Blpop:
    case Command::Brpop:
    case Command::Brpoplpush:
    case Command::Punsubscribe:
    case Command::Unsubscribe:
        c->setBlockRequest(false);
        break;
    case Command::Watch:
        if (res->isOk()) {
            c->incrWatch();
        } else {
            c->decrPendWatch();
        }
        break;
    case Command::Unwatch:
        if (res->isOk()) {
            c->unwatch();
        }
        c->setBlockRequest(false);
        break;
    case Command::Multi:
        if (res->isOk()) {
            c->incrMulti();
        } else {
            c->decrPendMulti();
        }
        break;
    case Command::Exec:
    case Command::Discard:
        if (c->inMulti()) {
            if (s) {
                c->unwatch();
                c->decrMulti();
            } else {
                addPostEvent(c, Multiplexor::ErrorEvent);
            }
        }
        c->setBlockRequest(false);
        break;
    case Command::Psubscribe:
    case Command::Subscribe:
        if (!s) {
            c->decrPendSub();
        }
        break;
    default:
        break;
    }
    if (s && !s->isShared()) {
        if (!c->inTransaction() && !c->inSub(true)) {
            mConnPool[s->server()->id()]->putPrivateConnection(s);
            c->detachConnectConnection();
            s->detachAcceptConnection();
        }
    }
}

void Handler::infoRequest(Request* req, const String& key)
{
    if (key.equal("ResetStats", true)) {
        mProxy->incrStatsVer();
        directResponse(req, Response::Ok);
        return;
    } else if (key.equal("Latency", true)) {
        infoLatencyRequest(req);
        return;
    } else if (key.equal("ServerLatency", true)) {
        infoServerLatencyRequest(req);
        return;
    }
    bool all = key.equal("All", true);
    bool empty = key.empty();
    ResponsePtr res = ResponseAlloc::create();
    res->setType(Reply::String);
    Segment& body = res->body();
    Buffer* buf = body.fset(nullptr, "");

#define Scope(all, empty, header) ((all || empty || key.equal(header, true)) ? \
        (buf = buf->fappend("# %s\n", header)) : nullptr)

    if (all || empty || key.equal("Proxy", true) || key.equal("Server", true)) {
        buf = buf->fappend("# %s\n", "Proxy");
        buf = buf->fappend("Version:%s\n", _PREDIXY_VERSION_);
        buf = buf->fappend("Name:%s\n", mProxy->conf()->name());
        buf = buf->fappend("Bind:%s\n", mProxy->conf()->bind());
        buf = buf->fappend("RedisMode:proxy\n");
#ifdef _PREDIXY_SINGLE_THREAD_
        buf = buf->fappend("SingleThread:true\n");
#else
        buf = buf->fappend("SingleThread:false\n");
#endif
        buf = buf->fappend("WorkerThreads:%d\n", mProxy->conf()->workerThreads());
        buf = buf->fappend("Uptime:%ld\n", (long)mProxy->startTime());
        SString<32> timeStr;
        timeStr.strftime("%Y-%m-%d %H:%M:%S", mProxy->startTime());
        buf = buf->fappend("UptimeSince:%s\n", timeStr.data());
        buf = buf->fappend("\n");
    }

    if (Scope(all, empty, "SystemResource")) {
        buf = buf->fappend("UsedMemory:%ld\n", AllocBase::getUsedMemory());
        buf = buf->fappend("MaxMemory:%ld\n", AllocBase::getMaxMemory());
        struct rusage ru;
        int ret = getrusage(RUSAGE_SELF, &ru);
        if (ret == 0) {
            buf = buf->fappend("MaxRSS:%ld\n", ru.ru_maxrss<<10);
            buf = buf->fappend("UsedCpuSys:%.3f\n", (double)ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1000000.);
            buf = buf->fappend("UsedCpuUser:%.3f\n", (double)ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1000000.);
        } else {
            logError("h %d getrusage fail %s", id(), StrError());
        }
        buf = buf->fappend("\n");
    }

    if (Scope(all, empty, "Stats")) {
        HandlerStats st(mStats);
        for (auto h : mProxy->handlers()) {
            if (h == this) {
                continue;
            }
            st += h->mStats;
        }
        buf = buf->fappend("Accept:%ld\n", st.accept);
        buf = buf->fappend("ClientConnections:%ld\n", st.clientConnections);
        buf = buf->fappend("TotalRequests:%ld\n", st.requests);
        buf = buf->fappend("TotalResponses:%ld\n", st.responses);
        buf = buf->fappend("TotalRecvClientBytes:%ld\n", st.recvClientBytes);
        buf = buf->fappend("TotalSendServerBytes:%ld\n", st.sendServerBytes);
        buf = buf->fappend("TotalRecvServerBytes:%ld\n", st.recvServerBytes);
        buf = buf->fappend("TotalSendClientBytes:%ld\n", st.sendClientBytes);
        buf = buf->fappend("\n");
    }

    if (Scope(all, empty, "Servers")) {
        int servCursor = 0;
        auto sp = mProxy->serverPool();
        while (Server* serv = sp->iter(servCursor)) {
            ServerStats st;
            for (auto h : mProxy->handlers()) {
                if (auto cp = h->getConnectConnectionPool(serv->id())) {
                    st += cp->stats();
                }
            }
            buf = buf->fappend("Server:%s\n", serv->addr().data());
            buf = buf->fappend("Role:%s\n", serv->roleStr());
            auto g = serv->group();
            buf = buf->fappend("Group:%s\n", g ? g->name().data() : "");
            buf = buf->fappend("DC:%s\n", serv->dcName().data());
            buf = buf->fappend("CurrentIsFail:%d\n", (int)serv->fail());
            buf = buf->fappend("Connections:%d\n", st.connections);
            buf = buf->fappend("Connect:%ld\n", st.connect);
            buf = buf->fappend("Requests:%ld\n", st.requests);
            buf = buf->fappend("Responses:%ld\n", st.responses);
            buf = buf->fappend("SendBytes:%ld\n", st.sendBytes);
            buf = buf->fappend("RecvBytes:%ld\n", st.recvBytes);
            buf = buf->fappend("\n");
        }
        buf = buf->fappend("\n");
    }

    if (Scope(all, empty, "LatencyMonitor")) {
        LatencyMonitor lm;
        for (size_t i = 0; i < mLatencyMonitors.size(); ++i) {
            lm = mLatencyMonitors[i];
            for (auto h : mProxy->handlers()) {
                if (h == this) {
                    continue;
                }
                lm += h->mLatencyMonitors[i];
            }
            buf = buf->fappend("LatencyMonitorName:%s\n", lm.name().data());
            buf = lm.output(buf);
            buf = buf->fappend("\n");
        }
    }

    buf = buf->fappend("\r\n");
    body.end().buf = buf;
    body.end().pos = buf->length();
    body.rewind();
    res->head().fset(nullptr, "$%d\r\n", body.length() - 2);
    handleResponse(nullptr, req, res);
}

void Handler::infoLatencyRequest(Request* req)
{
    SegmentStr<128> d(req->body());
    if (!d.hasPrefix("*3\r\n")) {
        directResponse(req, Response::ArgWrong);
        return;
    }
    const char* p = d.data() + sizeof("*3\r\n$4\r\ninfo\r\n$7\r\nlatency\r\n");
    int len = atoi(p);
    p = strchr(p, '\r') + 2;
    String key(p, len);
    ResponsePtr res = ResponseAlloc::create();
    Segment& body = res->body();
    int i = mProxy->latencyMonitorSet().find(key);
    if (i < 0) {
        res->setType(Reply::Error);
        body.fset(nullptr, "-ERR latency \"%.*s\" no exists\r\n", key.length(), key.data());
        handleResponse(nullptr, req, res);
        return;
    }

    BufferPtr buf = body.fset(nullptr, "# LatencyMonitor\n");
    LatencyMonitor lm = mLatencyMonitors[i];
    for (auto h : mProxy->handlers()) {
        if (h == this) {
            continue;
        }
        lm += h->mLatencyMonitors[i];
    }
    buf = buf->fappend("LatencyMonitorName:%s\n", lm.name().data());
    buf = lm.output(buf);
    buf = buf->fappend("\n");

    buf = buf->fappend("# ServerLatencyMonitor\n");
    auto sp = mProxy->serverPool();
    int servCursor = 0;
    while (Server* serv = sp->iter(servCursor)) {
        lm = mLatencyMonitors[i];
        lm.reset();
        for (auto h : mProxy->handlers()) {
            if (auto cp = h->getConnectConnectionPool(serv->id())) {
                lm += cp->latencyMonitors()[i];
            }
        }
        buf = buf->fappend("ServerLatencyMonitorName:%s %s\n",
                        serv->addr().data(), lm.name().data());
        buf = lm.output(buf);
        buf = buf->fappend("\n");
    }

    buf = buf->fappend("\r\n");
    body.end().buf = buf;
    body.end().pos = buf->length();
    body.rewind();
    res->head().fset(nullptr, "$%d\r\n", body.length() - 2);
    res->setType(Reply::String);
    handleResponse(nullptr, req, res);
}

void Handler::infoServerLatencyRequest(Request* req)
{
    SegmentStr<256> d(req->body());
    int argc = atoi(d.data() + 1);
    if (argc != 3 && argc != 4) {
        directResponse(req, Response::ArgWrong);
        return;
    }
    const char* p = d.data() + sizeof("*3\r\n$4\r\ninfo\r\n$13\r\nserverlatency\r\n");
    int len = atoi(p);
    p = strchr(p, '\r') + 2;
    String addr(p, len);
    ResponsePtr res = ResponseAlloc::create();
    Segment& body = res->body();
    auto sp = mProxy->serverPool();
    Server* serv = sp->getServer(addr);
    if (!serv) {
        res->setType(Reply::Error);
        body.fset(nullptr, "-ERR server \"%.*s\" no exists\r\n",
                addr.length(), addr.data());
        handleResponse(nullptr, req, res);
        return;
    }
    BufferPtr buf = body.fset(nullptr, "# ServerLatencyMonitor\n");
    if (argc == 4) {
        p += len + 3;
        len = atoi(p);
        p = strchr(p, '\r') + 2;
        String key(p, len);
        int i = mProxy->latencyMonitorSet().find(key);
        if (i < 0) {
            res->setType(Reply::Error);
            body.fset(nullptr, "-ERR latency \"%.*s\" no exists\r\n",
                    key.length(), key.data());
            handleResponse(nullptr, req, res);
            return;
        }
        LatencyMonitor lm = mLatencyMonitors[i];
        lm.reset();
        for (auto h : mProxy->handlers()) {
            if (auto cp = h->getConnectConnectionPool(serv->id())) {
                lm += cp->latencyMonitors()[i];
            }
        }
        buf = buf->fappend("ServerLatencyMonitorName:%s %s\n",
                        serv->addr().data(), lm.name().data());
        buf = lm.output(buf);
        buf = buf->fappend("\n");
    } else {
        for (size_t i = 0; i < mLatencyMonitors.size(); ++i) {
            LatencyMonitor lm = mLatencyMonitors[i];
            lm.reset();
            for (auto h : mProxy->handlers()) {
                if (auto cp = h->getConnectConnectionPool(serv->id())) {
                    lm += cp->latencyMonitors()[i];
                }
            }
            buf = buf->fappend("ServerLatencyMonitorName:%s %s\n",
                            serv->addr().data(), lm.name().data());
            buf = lm.output(buf);
            buf = buf->fappend("\n");
        }
    }

    buf = buf->fappend("\r\n");
    body.end().buf = buf;
    body.end().pos = buf->length();
    body.rewind();
    res->head().fset(nullptr, "$%d\r\n", body.length() - 2);
    res->setType(Reply::String);
    handleResponse(nullptr, req, res);

}

void Handler::resetStats()
{
    mStats.reset();
    for (auto& m : mLatencyMonitors) {
        m.reset();
    }
    for (auto cp : mConnPool) {
        if (cp) {
            cp->resetStats();
        }
    }
    mStatsVer = mProxy->statsVer();
}

void Handler::configRequest(Request* req, const String& key)
{
    if (key.equal("get", true)) {
        configGetRequest(req);
    } else if (key.equal("set", true)) {
        configSetRequest(req);
    } else if (key.equal("resetstat", true)) {
        mProxy->incrStatsVer();
        directResponse(req, Response::Ok);
    } else {
        directResponse(req, Response::ConfigSubCmdUnknown);
    }
}

void Handler::configGetRequest(Request* req)
{
    SegmentStr<128> d(req->body());
    if (!d.hasPrefix("*3\r\n")) {
        directResponse(req, Response::ArgWrong);
        return;
    }
    const char* p = d.data() + sizeof("*3\r\n$6\r\nconfig\r\n$3\r\nget\r\n");
    int len = atoi(p);
    p = strchr(p, '\r');
    String key(p + 2, len);
    bool all = key.equal("*");
    ResponsePtr res = ResponseAlloc::create();
    res->setType(Reply::Array);
    int num = 0;
    Segment& body = res->body();
    BufferPtr buf = BufferAlloc::create();
    body.begin().buf = buf;
    body.begin().pos = buf->length();
    SString<512> s;
    auto conf = mProxy->conf();
    auto log = Logger::gInst;

#define Append(name, fmt, ...) \
    if (all || key.equal(name, true)) {                             \
        buf = buf->fappend("$%d\r\n%s\r\n", sizeof(name) - 1, name);\
        s.printf(fmt, __VA_ARGS__);                                 \
        buf = buf->fappend("$%d\r\n%s\r\n", s.length(), s.data());  \
        num += 2;                                                   \
        if (!all) break;                                            \
    }

    do {
        Append("Name", "%s", conf->name());
        Append("Bind", "%s", conf->bind());
        Append("WorkerThreads", "%d", conf->workerThreads());
        Append("BufSize", "%d", Buffer::getSize() + sizeof(Buffer));
        Append("LocalDC", "%s", conf->localDC().c_str());
        Append("MaxMemory", "%ld", AllocBase::getMaxMemory());
        Append("ClientTimeout", "%d", conf->clientTimeout() / 1000000);
        Append("AllowMissLog", "%s", log->allowMissLog() ? "true" : "false");
        Append("LogVerbSample", "%d", log->logSample(LogLevel::Verb));
        Append("LogDebugSample", "%d", log->logSample(LogLevel::Debug));
        Append("LogInfoSample", "%d", log->logSample(LogLevel::Info));
        Append("LogNoticeSample", "%d", log->logSample(LogLevel::Notice));
        Append("LogWarnSample", "%d", log->logSample(LogLevel::Warn));
        Append("LogErrorSample", "%d", log->logSample(LogLevel::Error));
    } while (0);
    body.end().buf = buf;
    body.end().pos = buf->length();
    body.rewind();
    res->head().fset(nullptr, "*%d\r\n", num);
    handleResponse(nullptr, req, res);
}

void Handler::configSetRequest(Request* req)
{
    SegmentStr<128> d(req->body());
    int argc = atoi(d.data() + 1);
    if (argc < 4) {
        directResponse(req, Response::ArgWrong);
        return;
    }
    auto conf = mProxy->conf();
    auto log = Logger::gInst;
    char* p = strchr((char*)d.data(), '\r');
    p += sizeof("\r\n$6\r\nconfig\r\n$3\r\nset\r\n");
    int len = atoi(p);
    p = strchr(p, '\r') + 2;
    String key(p, len);
    p += len + 3;
    len = atoi(p);
    p = strchr(p, '\r') + 2;
    p[len] = '\0';
    String val(p, len);
    if (key.equal("MaxMemory", true)) {
        long m;
        if (Conf::parseMemory(m, val.data())) {
            AllocBase::setMaxMemory(m);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("ClientTimeout", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            conf->setClientTimeout(v * 1000000L);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("LogVerbSample", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            log->setLogSample(LogLevel::Verb, v);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("LogDebugSample", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            log->setLogSample(LogLevel::Debug, v);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("LogInfoSample", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            log->setLogSample(LogLevel::Info, v);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("LogNoticeSample", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            log->setLogSample(LogLevel::Notice, v);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("LogWarnSample", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            log->setLogSample(LogLevel::Warn, v);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("LogErrorSample", true)) {
        int v;
        if (sscanf(val.data(), "%d", &v) == 1 && v >= 0) {
            log->setLogSample(LogLevel::Error, v);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else if (key.equal("AllowMissLog", true)) {
        if (val.equal("true")) {
            log->setAllowMissLog(true);
            directResponse(req, Response::Ok);
        } else if (val.equal("false")) {
            log->setAllowMissLog(false);
            directResponse(req, Response::Ok);
        } else {
            directResponse(req, Response::ArgWrong);
        }
    } else {
        directResponse(req, Response::ArgWrong);
    }
}

void Handler::innerResponse(ConnectConnection* s, Request* req, Response* res)
{
    logDebug("h %d s %s %d inner req %ld %s res %ld %s",
            id(), (s ? s->peer() : "None"), (s ? s->fd() : -1),
            req->id(), req->cmd(),
            res->id(), res->typeStr());
    switch (req->type()) {
    case Command::PingServ:
        if (s && res->isPong()) {
            Server* serv = s->server();
            if (serv->fail()) {
                logError("ibk: MARK SERVER NOT FAIL BASED ON THE PING");
                serv->setFail(false); // if server respond to ping, mark fail as false. TODO: add for other case like loading
                logNotice("h %d s %s %d mark server alive",
                        id(), s->peer(), s->fd());
            }
        }
        break;
    case Command::AuthServ:
        if (!res->isOk()) {
            s->setStatus(ConnectConnection::LogicError);
            addPostEvent(s, Multiplexor::ErrorEvent);
            logWarn("h %d s %s %d auth fail",
                    id(), s->peer(), s->fd());
        }
        break;
    case Command::Readonly: // when loading, predixy can detect it here
        if (!res->isOk()) {
            s->setStatus(ConnectConnection::LogicError);
            addPostEvent(s, Multiplexor::ErrorEvent);
            logWarn("h %d s %s %d readonly fail",
                    id(), s->peer(), s->fd());
        }
        break;
    case Command::SelectServ:
        if (!res->isOk()) {
            s->setStatus(ConnectConnection::LogicError);
            addPostEvent(s, Multiplexor::ErrorEvent);
            logWarn("h %d s %s %d db select %d fail",
                    id(), s->peer(), s->fd(), s->db());
        }
        break;
    case Command::ClusterNodes:
    case Command::SentinelSentinels:
    case Command::SentinelGetMaster:
    case Command::SentinelSlaves:
        mProxy->serverPool()->handleResponse(this, s, req, res);
        break;
    case Command::UnwatchServ:
        if (!res->isOk()) {
            s->setStatus(ConnectConnection::LogicError);
            addPostEvent(s, Multiplexor::ErrorEvent);
            logWarn("h %d s %s %d unwatch fail",
                    id(), s->peer(), s->fd(), s->db());
        }
        break;
    case Command::DiscardServ:
        if (!res->isOk()) {
            s->setStatus(ConnectConnection::LogicError);
            addPostEvent(s, Multiplexor::ErrorEvent);
            logWarn("h %d s %s %d discard fail",
                    id(), s->peer(), s->fd(), s->db());
        }
        break;
    default:
        break;
    }
}

bool Handler::redirect(ConnectConnection* c, Request* req, Response* res, bool moveOrAsk)
{
    FuncCallTimer();
    if (req->incrRedirectCnt() > Request::MaxRedirectLimit) {
        return false;
    }
    int slot;
    SString<Const::MaxAddrLen> addr;
    if (moveOrAsk) {
        if (!res->getMoved(slot, addr)) {
            return false;
        }
    } else {
        if (!res->getAsk(addr)) {
            return false;
        }
    }
    auto p = static_cast<ClusterServerPool*>(mProxy->serverPool());
    Server* serv = p->redirect(addr, c->server());
    if (!serv) {
        logDebug("h %d req %ld %s redirect to %s can't get server",
                id(), req->id(), (moveOrAsk ? "MOVE" : "ASK"), addr.data());
        return false;
    }
    req->rewind();
    auto s = getConnectConnection(req, serv);
    if (!s) {
        return false;
    }
    logDebug("h %d %s redirect req %ld from %s %d to %s",
              id(), (moveOrAsk ? "MOVE" : "ASK"),
              req->id(), c->peer(), c->fd(), addr.data());
    if (!moveOrAsk) {
        RequestPtr asking = RequestAlloc::create(Request::Asking);
        handleRequest(asking, s);
    }
    handleRequest(req, s);
    return true;
}

bool Handler::permission(Request* req, const String& key, Response::GenericCode& code)
{
    FuncCallTimer();
    AcceptConnection* c = req->connection();
    if (!c) {
        return true;
    }
    if (req->type() == Command::Auth) {
        SString<Const::MaxKeyLen> pw;
        if (req->isInline()) {
            RequestParser::decodeInlineArg(pw, key);
        }
        auto m = mProxy->authority();
        if (!m->hasAuth()) {
            code = Response::NoPasswordSet;
        } else if (auto auth = m->get(req->isInline() ? pw : key)) {
            c->setAuth(auth);
            code = Response::Ok;
        } else {
            logNotice("h %d c %s %d auth '%.*s' invalid",
                    id(), c->peer(), c->fd(), key.length(), key.data());
            c->setAuth(m->getDefault());
            code = Response::InvalidPassword;
        }
        return false;
    }
    auto a = c->auth();
    if (!a) {
        code = Response::Unauth;
        return false;
    }
    if (!a->permission(req, key)) {
        code = Response::PermissionDeny;
        return false;
    }
    return true;
}
