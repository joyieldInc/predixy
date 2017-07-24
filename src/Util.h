/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_UTIL_H_
#define _PREDIXY_UTIL_H_

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <chrono>

template<class T>
class PtrObjCmp
{
public:
    bool operator()(const T* p1, const T* p2) const
    {
        return *p1 < *p2;
    }
};

class StrErrorImpl
{
public:
    StrErrorImpl()
    {
        set(errno);
    }
    StrErrorImpl(int err)
    {
        set(err);
    }
    void set(int err)
    {
#if _GNU_SOURCE
        mStr = strerror_r(err, mBuf, sizeof(mBuf));
#else
        strerror_r(err, mBuf, sizeof(mBuf));
        mStr = mBuf;
#endif
    }
    const char* str() const
    {
        return mStr;
    }
private:
    char* mStr;
    char mBuf[256];
};

#define StrError(...) StrErrorImpl(__VA_ARGS__).str()

namespace Util
{
    using namespace std::chrono;
    inline long nowSec()
    {
        return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    }
    inline long nowMSec()
    {
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
    inline long nowUSec()
    {
        return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    }
    inline long elapsedSec()
    {
        return duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
    }
    inline long elapsedMSec()
    {
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
    inline long elapsedUSec()
    {
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }
};

#endif
