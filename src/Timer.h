/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_TIMER_H_
#define _PREDIXY_TIMER_H_

#include <map>
#include <chrono>
#include "Sync.h"

class TimerPoint
{
public:
    TimerPoint(const char* key);
    const char* key() const
    {
        return mKey;
    }
    long elapsed() const
    {
        return mElapsed;
    }
    long count() const
    {
        return mCount;
    }
    void add(long v)
    {
        mElapsed += v;
        ++mCount;
    }
    static void report();
private:
    const char* mKey;
    AtomicLong mElapsed;
    AtomicLong mCount;
    TimerPoint* mNext;
private:
    static Mutex Mtx;
    static TimerPoint* Head;
    static TimerPoint* Tail;
    static int PointCnt;
};

class Timer
{
public:
    Timer():
        mKey(nullptr),
        mStart(now())
    {
    }
    Timer(TimerPoint* key):
        mKey(key),
        mStart(now())
    {
    }
    ~Timer()
    {
        if (mKey) {
            long v = elapsed();
            if (v >= 0) {
                mKey->add(v);
            }
        }
    }
    void restart()
    {
        mStart = now();
    }
    long stop()
    {
        long ret = elapsed();
        mStart = -1;
        if (ret >= 0 && mKey) {
            mKey->add(ret);
        }
        return ret;
    }
    long elapsed() const
    {
        return mStart < 0 ? -1 : now() - mStart;
    }
private:
    static long now()
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }
private:
    TimerPoint* mKey;
    long mStart;
};

#ifdef _PREDIXY_TIMER_STATS_

#define FuncCallTimer() static TimerPoint _timer_point_tttt_(__PRETTY_FUNCTION__);\
    Timer _tiemr_tttt_(&_timer_point_tttt_)

#else

#define FuncCallTimer() 

#endif

#endif
