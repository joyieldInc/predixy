/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "LatencyMonitor.h"

Buffer* LatencyMonitor::output(Buffer* buf) const
{
    long tc = mLast.count;
    for (auto& s : mTimeSpan) {
        tc += s.count;
    }
    if (tc == 0) {
        return buf;
    }
    long te = mLast.total;
    long t = 0;
    float p = 0;
    for (auto& s : mTimeSpan) {
        if (s.count == 0) {
            continue;
        }
        te += s.total;
        t += s.count;
        p = t * 100. / tc;
        buf = buf->fappend("<= %12ld %20ld %16ld %.2f%%\n", s.span, s.total, s.count, p);
    }
    if (mLast.count > 0) {
        buf = buf->fappend(">  %12ld %20ld %16ld 100.00%\n", mLast.span, mLast.total, mLast.count);
    }
    buf = buf->fappend("T  %12ld %20ld %16ld\n", te / tc, te, tc);
    return buf;
}


void LatencyMonitorSet::init(const std::vector<LatencyMonitorConf>& conf)
{
    mPool.resize(conf.size());
    int i = 0;
    for (auto& c : conf) {
        if (mNameIdx.find(c.name) != mNameIdx.end()) {
            Throw(DuplicateDef, "LatencyMonitor \"%s\" duplicate",
                  c.name.c_str());
        }
        mPool[i].init(c);
        mNameIdx[mPool[i].name()] = i;
        ++i;
    }
    mCmdIdx.resize(Command::Sentinel);
    int cursor = 0;
    while (auto cmd = Command::iter(cursor)) {
        int size = conf.size();
        for (int i = 0; i < size; ++i) {
            if (conf[i].cmds[cmd->type]) {
                mCmdIdx[cmd->type].push_back(i);
            }
        }
    }
}
