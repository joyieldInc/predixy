/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_LATENCY_MONITOR_H_
#define _PREDIXY_LATENCY_MONITOR_H_

#include <algorithm>
#include <vector>
#include <map>
#include "String.h"
#include "Buffer.h"
#include "Conf.h"

class LatencyMonitor
{
public:
    struct TimeSpan
    {
        long span;
        long total;
        long count;

        bool operator<(const TimeSpan& oth) const
        {
            return span < oth.span;
        }
        TimeSpan& operator+=(const TimeSpan& s)
        {
            total += s.total;
            count += s.count;
            return *this;
        }
    };
public:
    LatencyMonitor():
        mCmds(nullptr),
        mLast{0, 0, 0}
    {
    }
    LatencyMonitor& operator+=(const LatencyMonitor& m)
    {
        for (size_t i = 0; i < mTimeSpan.size(); ++i) {
            mTimeSpan[i] += m.mTimeSpan[i];
        }
        mLast += m.mLast;
        return *this;
    }
    void init(const LatencyMonitorConf& c)
    {
        mName = c.name;
        mCmds = &c.cmds;
        mTimeSpan.resize(c.timeSpan.size());
        for (size_t i = 0; i < mTimeSpan.size(); ++i) {
            mTimeSpan[i].span = c.timeSpan[i];
            mTimeSpan[i].total = 0;
            mTimeSpan[i].count = 0;
        }
        if (!mTimeSpan.empty()) {
            mLast.span = mTimeSpan.back().span;
        }
    }
    void reset()
    {
        for (auto& s : mTimeSpan) {
            s.total = 0;
            s.count = 0;
        }
        mLast.total = 0;
        mLast.count = 0;
    }
    const String& name() const
    {
        return mName;
    }
    int add(long v)
    {
        TimeSpan span{v};
        auto it = std::lower_bound(mTimeSpan.begin(), mTimeSpan.end(), span);
        if (it == mTimeSpan.end()) {
            mLast.total += v;
            ++mLast.count;
            return mTimeSpan.size();
        } else {
            it->total += v;
            ++it->count;
            return it - mTimeSpan.begin();
        }
    }
    void add(long v, int idx)
    {
        TimeSpan& s(idx < (int)mTimeSpan.size() ? mTimeSpan[idx] : mLast);
        s.total += v;
        ++s.count;
    }
    Buffer* output(Buffer* buf) const;
private:
    String mName;
    const std::bitset<Command::AvailableCommands>* mCmds;
    std::vector<TimeSpan> mTimeSpan;
    TimeSpan mLast;
};

class LatencyMonitorSet
{
public:
    DefException(DuplicateDef);
public:
    LatencyMonitorSet()
    {
    }
    void init(const std::vector<LatencyMonitorConf>& conf);
    const std::vector<LatencyMonitor>& latencyMonitors() const
    {
        return mPool;
    }
    int find(const String& name) const
    {
        auto it = mNameIdx.find(name);
        return it == mNameIdx.end() ? -1 : it->second;
    }
    const std::vector<int>& cmdIndex(Command::Type type) const
    {
        return mCmdIdx[type];
    }
private:
    std::vector<LatencyMonitor> mPool;
    std::map<String, int> mNameIdx;
    std::vector<std::vector<int>> mCmdIdx;
};


#endif
