/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <algorithm>
#include "Logger.h"
#include "ServerGroup.h"
#include "Handler.h"
#include "StandaloneServerPool.h"

StandaloneServerPool::StandaloneServerPool(Proxy* p):
    ServerPoolTmpl(p, Standalone),
    mDist(Distribution::Modula)
{
    mSentinels.reserve(MaxSentinelNum);
    mServPool.reserve(Const::MaxServNum);
    mHashTag[0] = mHashTag[1] = '\0';
}

StandaloneServerPool::~StandaloneServerPool()
{
}

void StandaloneServerPool::init(const StandaloneServerPoolConf& conf)
{
    ServerPool::init(conf);
    mRefreshMethod = conf.refreshMethod;
    mDist = conf.dist;
    mHash = conf.hash;
    mHashTag[0] = conf.hashTag[0];
    mHashTag[1] = conf.hashTag[1];
    int i = 0;
    if (conf.refreshMethod == ServerPoolRefreshMethod::Sentinel) {
        mSentinels.resize(conf.sentinels.size());
        for (auto& sc : conf.sentinels) {
            Server* s = new Server(this, sc.addr, true);
            s->setRole(Server::Sentinel);
            s->setPassword(sc.password.empty() ? conf.sentinelPassword:sc.password);
            mSentinels[i++] = s;
            mServs[s->addr()] = s;
        }
    }
    mGroupPool.resize(conf.groups.size());
    i = 0;
    for (auto& gc : conf.groups) {
        ServerGroup* g = new ServerGroup(this, gc.name);
        mGroupPool[i++] = g;
        auto role = Server::Master;
        for (auto& sc : gc.servers) {
            Server* s = new Server(this, sc.addr, true);
            s->setPassword(sc.password.empty() ? conf.password : sc.password);
            mServPool.push_back(s);
            mServs[s->addr()] = s;
            g->add(s);
            s->setGroup(g);
            switch (mRefreshMethod.value()) {
            case ServerPoolRefreshMethod::Fixed:
                s->setOnline(true);
                s->setRole(role);
                role = Server::Slave;
                break;
            default:
                s->setOnline(false);
                break;
            }
        }
    }
}

Server* StandaloneServerPool::getServer(Handler* h, Request* req, const String& key) const
{
    FuncCallTimer();
    switch (req->type()) {
    case Command::SentinelGetMaster:
    case Command::SentinelSlaves:
    case Command::SentinelSentinels:
        if (mSentinels.empty()) {
            return nullptr;
        } else  {
            Server* s = randServer(h, mSentinels);
            logDebug("sentinel server pool get server %s for sentinel command",
                     s->addr().data());
            return s;
        }
        break;
    case Command::Randomkey:
        return randServer(h, mServPool);
    default:
        break;
    }
    if (mGroupPool.size() == 1) {
        return mGroupPool[0]->getServer(h, req);
    } else if (mGroupPool.size() > 1) {
        switch (mDist) {
        case Distribution::Modula:
            {
                long idx = mHash.hash(key.data(), key.length(), mHashTag);
                idx %= mGroupPool.size();
                return mGroupPool[idx]->getServer(h, req);
            }
            break;
        case Distribution::Random:
            {
                int idx = h->rand() % mGroupPool.size();
                return mGroupPool[idx]->getServer(h, req);
            }
            break;
        default:
            break;
        }
    }
    return nullptr;
}

void StandaloneServerPool::refreshRequest(Handler* h)
{
    logDebug("h %d update standalone server pool", h->id());
    switch (mRefreshMethod.value()) {
    case ServerPoolRefreshMethod::Sentinel:
        for (auto g : mGroupPool) {
            RequestPtr req = RequestAlloc::create();
            req->setSentinels(g->name());
            req->setData(g);
            h->handleRequest(req);
            req = RequestAlloc::create();
            req->setSentinelGetMaster(g->name());
            req->setData(g);
            h->handleRequest(req);
            req = RequestAlloc::create();
            req->setSentinelSlaves(g->name());
            req->setData(g);
            h->handleRequest(req);
        }
        break;
    default:
        break;
    }
}

void StandaloneServerPool::handleResponse(Handler* h, ConnectConnection* s, Request* req, Response* res)
{
    switch (req->type()) {
    case Command::SentinelSentinels:
        handleSentinels(h, s, req, res);
        break;
    case Command::SentinelGetMaster:
        handleGetMaster(h, s, req, res);
        break;
    case Command::SentinelSlaves:
        handleSlaves(h, s, req, res);
        break;
    default:
        break;
    }
}

class AddrParser
{
public:
    enum Status {
        Ok,
        Error,
        Done
    };
public:
    AddrParser(const Segment& res):
        mRes(res),
        mState(Idle),
        mCnt(0),
        mArgLen(0),
        mIp(false),
        mPort(false)
    {
        mRes.rewind();
    }
    int count() const {return mCnt;}
    Status parse(SString<Const::MaxAddrLen>& addr);
private:
    enum State {
        Idle,
        Count,
        CountLF,
        Arg,
        ArgLen,
        ArgLenLF,
        SubArrayLen,
        Body,
        BodyLF,
        Invalid,
        Finished
    };
private:
    Segment mRes;
    State mState;
    int mCnt;
    int mArgLen;
    bool mIp;
    bool mPort;
    SString<4> mKey;
};

AddrParser::Status AddrParser::parse(SString<Const::MaxAddrLen>& addr)
{
    const char* dat;
    int len;
    addr.clear();
    while (mRes.get(dat, len) && mState != Invalid) {
        for (int i = 0; i < len && mState != Invalid; ++i) {
            char ch = dat[i];
            switch (mState) {
            case Idle:
                mState = ch == '*' ? Count : Invalid;
                break;
            case Count:
                if (ch >= '0' && ch <= '9') {
                    mCnt = mCnt * 10 + (ch - '0');
                } else if (ch == '\r') {
                    if (mCnt == 0) {
                        mState = Finished;
                        return Done;
                    } else if (mCnt < 0) {
                        mState = Invalid;
                        return Error;
                    }
                    mState = CountLF;
                } else {
                    mState = Invalid;
                }
                break;
            case CountLF:
                mState = ch == '\n' ? Arg : Invalid;
                break;
            case Arg:
                if (ch == '$') {
                    mState = ArgLen;
                    mArgLen = 0;
                } else if (ch == '*') {
                    mState = SubArrayLen;
                } else {
                    mState = Invalid;
                }
                break;
            case ArgLen:
                if (ch >= '0' && ch <= '9') {
                    mArgLen = mArgLen * 10 + (ch - '0');
                } else if (ch == '\r') {
                    mState = ArgLenLF;
                } else {
                    mState = Invalid;
                }
                break;
            case ArgLenLF:
                mState = ch == '\n' ? Body : Invalid;
                break;
            case SubArrayLen:
                if (ch == '\n') {
                    mState = Arg;
                }
                break;
            case Body:
                if (ch == '\r') {
                    mState = BodyLF;
                    if (mPort) {
                        mPort = false;
                        mRes.use(i + 1);
                        return Ok;
                    } else if (mIp) {
                        mIp = false;
                        addr.append(':');
                    } else if (mArgLen == 2 && strcmp(mKey.data(), "ip") == 0) {
                        mIp = true;
                    } else if (mArgLen == 4 && strcmp(mKey.data(), "port") == 0) {
                        mPort = true;
                    }
                    break;
                }
                if (mIp || mPort) {
                    addr.append(ch);
                } else if (mArgLen == 2 || mArgLen == 4) {
                    mKey.append(ch);
                }
                break;
            case BodyLF:
                mKey.clear();
                mState = ch == '\n' ? Arg : Invalid;
                break;
            default:
                break;
            }
        }
        mRes.use(len);
    }
    return mState != Invalid ? Done : Error;
}

static bool hasValidPort(const String& addr)
{
    const char* p = addr.data() + addr.length();
    for (int i = 0; i < addr.length(); ++i) {
        if (*(--p) == ':') {
            int port = atoi(p + 1);
            return port > 0 && port < 65536;
        }
    }
    return false;
}

void StandaloneServerPool::handleSentinels(Handler* h, ConnectConnection* s, Request* req, Response* res)
{
    if (!res || !res->isArray()) {
        return;
    }
    AddrParser parser(res->body());
    SString<Const::MaxAddrLen> addr;
    while (true) {
        auto st = parser.parse(addr);
        if (st == AddrParser::Ok) {
            logDebug("sentinel server pool parse sentinel %s", addr.data());
            if (!hasValidPort(addr)) {
                logNotice("sentinel server pool parse sentienl %s invalid",
                        addr.data());
                continue;
            }
            auto it = mServs.find(addr);
            Server* serv = it == mServs.end() ? nullptr : it->second;
            if (!serv) {
                if (mSentinels.size() == mSentinels.capacity()) {
                    logWarn("too many sentinels %d, will ignore new sentinel %s",
                            (int)mSentinels.size(), addr.data());
                    continue;
                }
                serv = new Server(this, addr, false);
                serv->setRole(Server::Sentinel);
                serv->setPassword(password());
                mSentinels.push_back(serv);
                mServs[serv->addr()] = serv;
                logNotice("h %d create new sentinel %s",
                        h->id(), addr.data());
            }
            serv->setOnline(true);
        } else if (st == AddrParser::Done) {
            break;
        } else {
            logError("sentinel server pool parse sentinel sentinels error");
            break;
        }
    }
}

void StandaloneServerPool::handleGetMaster(Handler* h, ConnectConnection* s, Request* req, Response* res)
{
    if (!res || !res->isArray()) {
        return;
    }
    ServerGroup* g = (ServerGroup*)req->data();
    if (!g) {
        return;
    }
    SegmentStr<Const::MaxAddrLen + 32> str(res->body());
    if (!str.complete()) {
        return;
    }
    if (strncmp(str.data(), "*2\r\n$", 5) != 0) {
        return;
    }
    SString<Const::MaxAddrLen> addr;
    const char* p = str.data() + 5;
    int len = atoi(p);
    if (len <= 0) {
        return;
    }
    p = strchr(p, '\r') + 2;
    if (!addr.append(p, len)) {
        return;
    }
    if (!addr.append(':')) {
        return;
    }
    p += len + 3;
    len = atoi(p);
    if (len <= 0) {
        return;
    }
    p = strchr(p, '\r') + 2;
    if (!addr.append(p, len)) {
        return;
    }
    logDebug("sentinel server pool group %s get master %s",
             g->name().data(), addr.data());
    auto it = mServs.find(addr);
    Server* serv = it == mServs.end() ? nullptr : it->second;
    if (serv) {
        serv->setOnline(true);
        serv->setRole(Server::Master);
        auto old = serv->group();
        if (old) {
            if (old != g) {
                old->remove(serv);
                g->add(serv);
                serv->setGroup(g);
            }
        } else {
            g->add(serv);
            serv->setGroup(g);
        }
    } else {
        if (mServPool.size() == mServPool.capacity()) {
            logWarn("too many servers %d, will ignore new master server %s",
                     (int)mServPool.size(), addr.data());
            return;
        }
        serv = new Server(this, addr, false);
        serv->setRole(Server::Master);
        serv->setPassword(password());
        mServPool.push_back(serv);
        g->add(serv);
        serv->setGroup(g);
        mServs[serv->addr()] = serv;
        logNotice("sentinel server pool group %s create master server %s %s",
                  g->name().data(), addr.data(), serv->dcName().data());
    }
}

void StandaloneServerPool::handleSlaves(Handler* h, ConnectConnection* s, Request* req, Response* res)
{
    if (!res || !res->isArray()) {
        return;
    }
    ServerGroup* g = (ServerGroup*)req->data();
    if (!g) {
        return;
    }
    AddrParser parser(res->body());
    SString<Const::MaxAddrLen> addr;
    while (true) {
        auto st = parser.parse(addr);
        if (st == AddrParser::Ok) {
            logDebug("sentinel server pool group %s parse slave %s",
                     g->name().data(), addr.data());
            auto it = mServs.find(addr);
            Server* serv = it == mServs.end() ? nullptr : it->second;
            if (serv) {
                serv->setOnline(true);
                serv->setRole(Server::Slave);
                auto old = serv->group();
                if (old) {
                    if (old != g) {
                        old->remove(serv);
                        g->add(serv);
                        serv->setGroup(g);
                    }
                } else {
                    g->add(serv);
                    serv->setGroup(g);
                }
            } else {
                if (mServPool.size() == mServPool.capacity()) {
                    logWarn("too many servers %d, will ignore new slave server %s",
                            (int)mServPool.size(), addr.data());
                    return;
                }
                serv = new Server(this, addr, false);
                serv->setRole(Server::Slave);
                serv->setPassword(password());
                mServPool.push_back(serv);
                g->add(serv);
                serv->setGroup(g);
                mServs[serv->addr()] = serv;
                logNotice("sentinel server pool group %s create slave server %s %s",
                          g->name().data(), addr.data(), serv->dcName().data());
            }
        } else if (st == AddrParser::Done) {
            break;
        } else {
            logError("sentinel server pool group %s parse sentinel sentinels error",
                    g->name().data());
            break;
        }
    }
}
