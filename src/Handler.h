/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_HANDLER_H_
#define _PREDIXY_HANDLER_H_

#include <vector>
#include "Predixy.h"
#include "Multiplexor.h"
#include "Stats.h"
#include "LatencyMonitor.h"
#include "AcceptConnection.h"
#include "ConnectConnectionPool.h"
#include "Proxy.h"

class Handler : public ID<Handler>
{
public:
    DefException(AddListenerEventFail);
public:
    Handler(Proxy* p);
    ~Handler();
    void run();
    void stop();
    void handleEvent(Socket* s, int evts);
    void handleRequest(Request* req);
    void handleRequest(Request* req, ConnectConnection* c);
    void handleResponse(ConnectConnection* c, Request* req, Response* res);
    void handleResponse(ConnectConnection* c, Request* req, Response::GenericCode code);
    void directResponse(Request* req, Response::GenericCode code, ConnectConnection* s=nullptr);
    Proxy* proxy() const
    {
        return mProxy;
    }
    Multiplexor* eventLoop() const
    {
        return mEventLoop;
    }
    HandlerStats& stats()
    {
        return mStats;
    }
    const HandlerStats& stats() const
    {
        return mStats;
    }
    const std::vector<LatencyMonitor>& latencyMonitors() const
    {
        return mLatencyMonitors;
    }
    ConnectConnectionPool* getConnectConnectionPool(int id) const
    {
        return id < (int)mConnPool.size() ? mConnPool[id] : nullptr;
    }
    int getPendRequests(Server* serv) const
    {
        if (auto cp = getConnectConnectionPool(serv->id())) {
            return cp->pendRequests();
        }
        return 0;
    }
    void addServerReadStats(Server* serv, int num)
    {
        mStats.recvServerBytes += num;
        mConnPool[serv->id()]->stats().recvBytes += num;
    }
    void addServerWriteStats(Server* serv, int num)
    {
        mStats.sendServerBytes += num;
        mConnPool[serv->id()]->stats().sendBytes += num;
    }
    IDUnique& idUnique()
    {
        return mIDUnique;
    }
    int rand()
    {
        return rand_r(&mRandSeed);
    }
private:
    bool preHandleRequest(Request* req, const String& key);
    void postHandleRequest(Request* req, ConnectConnection* s);
    void addPostEvent(AcceptConnection* c, int evts);
    void addPostEvent(ConnectConnection* c, int evts);
    void postEvent();
    void handleListenEvent(ListenSocket* s, int evts);
    void addAcceptSocket(int c, sockaddr* addr, socklen_t len);
    void handleAcceptConnectionEvent(AcceptConnection* c, int evts);
    void handleConnectConnectionEvent(ConnectConnection* c, int evts);
    void postAcceptConnectionEvent();
    void postConnectConnectionEvent();
    ConnectConnection* getConnectConnection(Request* req, Server* s);
    void refreshServerPool();
    void checkConnectionPool();
    int checkClientTimeout(long timeout);
    int checkServerTimeout(long timeout);
    void innerResponse(ConnectConnection* c, Request* req, Response* res);
    void infoRequest(Request* req, const String& key);
    void infoLatencyRequest(Request* req);
    void infoServerLatencyRequest(Request* req);
    void configRequest(Request* req, const String& key);
    void configGetRequest(Request* req);
    void configSetRequest(Request* req);
    bool redirect(ConnectConnection* c, Request* req, Response* res, bool moveOrAsk);
    bool permission(Request* req, const String& key, Response::GenericCode& code);
    void resetStats();
    void setAcceptConnectionActiveTime(AcceptConnection* c)
    {
        c->setLastActiveTime(Util::elapsedUSec());
        mAcceptConns.remove(c);
        mAcceptConns.push_back(c);
    }
private:
    bool mStop;
    Proxy* mProxy;
    Multiplexor* mEventLoop;
    std::vector<ConnectConnectionPool*> mConnPool;
    AcceptConnectionDeque mAcceptConns;
    AcceptConnectionList mPostAcceptConns;
    ConnectConnectionList mPostConnectConns;
    ConnectConnectionDeque mWaitConnectConns;
    long mStatsVer;
    HandlerStats mStats;
    std::vector<LatencyMonitor> mLatencyMonitors;
    IDUnique mIDUnique;
    unsigned int mRandSeed;
};


#endif
