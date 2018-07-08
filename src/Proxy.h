/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_PROXY_H_
#define _PREDIXY_PROXY_H_

#include <vector>
#include "Predixy.h"
#include "Handler.h"
#include "DC.h"
#include "ServerPool.h"
#include "ClusterServerPool.h"
#include "StandaloneServerPool.h"
#include "LatencyMonitor.h"

class Proxy
{
public:
    DefException(InitFail);
public:
    Proxy();
    ~Proxy();
    bool init(int argc, char* argv[]);
    int run();
    time_t startTime() const
    {
        return mStartTime;
    }
    Conf* conf() const
    {
        return mConf;
    }
    ListenSocket* listener() const
    {
        return mListener;
    }
    const Authority* authority() const
    {
        return &mAuthority;
    }
    DataCenter* dataCenter() const
    {
        return mDataCenter;
    }
    ServerPool* serverPool() const
    {
        return mServPool;
    }
    bool isSplitMultiKey() const
    {
        return mConf->standaloneServerPool().groups.size() != 1;
    }
    bool supportTransaction() const
    {
        return mConf->standaloneServerPool().groups.size() == 1;
    }
    bool supportSubscribe() const
    {
        return mConf->standaloneServerPool().groups.size() == 1 ||
               mConf->clusterServerPool().servers.size() > 0;
    }
    const std::vector<Handler*>& handlers() const
    {
        return mHandlers;
    }
    long statsVer() const
    {
        return mStatsVer;
    }
    long incrStatsVer()
    {
        return ++mStatsVer;
    }
    const LatencyMonitorSet& latencyMonitorSet() const
    {
        return mLatencyMonitorSet;
    }
private:
    Conf* mConf;
    ListenSocket* mListener;
    Authority mAuthority;
    DataCenter* mDataCenter;
    std::vector<Handler*> mHandlers;
    ServerPool* mServPool;
    time_t mStartTime;
    AtomicLong mStatsVer;
    LatencyMonitorSet mLatencyMonitorSet;
};


#endif
