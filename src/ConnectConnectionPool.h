/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CONNECT_CONNECTION_POOL_H_
#define _PREDIXY_CONNECT_CONNECTION_POOL_H_

#include <vector>
#include "ConnectConnection.h"
#include "Server.h"
#include "Handler.h"
#include "Request.h"
#include "Predixy.h"
#include "Stats.h"
#include "LatencyMonitor.h"

class ConnectConnectionPool
{
public:
    ConnectConnectionPool(Handler* h, Server* s, int dbnum);
    ~ConnectConnectionPool();
    ConnectConnection* getShareConnection(int db=0);
    ConnectConnection* getPrivateConnection(int db=0);
    void putPrivateConnection(ConnectConnection* s);
    void check();
    Server* server() const
    {
        return mServ;
    }
    int pendRequests()
    {
        return mPendRequests;
    }
    int incrPendRequests()
    {
        return ++mPendRequests;
    }
    int decrPendRequests()
    {
        return --mPendRequests;
    }
    ServerStats& stats()
    {
        return mStats;
    }
    const ServerStats& stats() const
    {
        return mStats;
    }
    std::vector<LatencyMonitor>& latencyMonitors()
    {
        return mLatencyMonitors;
    }
    const std::vector<LatencyMonitor>& latencyMonitors() const
    {
        return mLatencyMonitors;
    }
    void resetStats()
    {
        mStats.reset();
        for (auto& m : mLatencyMonitors) {
            m.reset();
        }
    }
private:
    bool init(ConnectConnection* c);
private:
    Handler* mHandler;
    Server* mServ;
    int mPendRequests;
    std::vector<ConnectConnection*> mShareConns;
    std::vector<ConnectConnectionList> mPrivateConns;
    ServerStats mStats;
    std::vector<LatencyMonitor> mLatencyMonitors;
};


#endif
