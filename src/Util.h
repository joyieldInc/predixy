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
    
    // Convert 128-bit unsigned integer to string
    // Returns the length of the converted string (excluding null terminator)
    // The buffer must be at least 40 bytes (39 digits + null terminator for max __uint128_t value)
    inline int uint128ToString(__uint128_t value, char* buf)
    {
        int n = 0;
        if (value == 0) {
            buf[n++] = '0';
        } else {
            char temp[64];
            int tempLen = 0;
            __uint128_t v = value;
            while (v > 0) {
                temp[tempLen++] = '0' + (v % 10);
                v /= 10;
            }
            // Reverse digits (extracted from low to high)
            for (int i = tempLen - 1; i >= 0; i--) {
                buf[n++] = temp[i];
            }
        }
        buf[n] = '\0';
        return n;
    }
};

#endif
