/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SUBSCRIBE_H_
#define _PREDIXY_SUBSCRIBE_H_

#include "Predixy.h"

class Subscribe
{
public:
    static const int16_t MaxSub = 32000;
public:
    Subscribe();
    int incrPendSub()
    {
        return mPendSub < MaxSub ? ++mPendSub : 0;
    }
    int decrPendSub()
    {
        return mPendSub > 0 ? --mPendSub : 0;
    }
    bool inPendSub() const
    {
        return mPendSub > 0;
    }
    void setSub(int chs)
    {
        mSub = chs;
    }
    bool inSub(bool includePend) const
    {
        return mSub > 0 || (includePend && mPendSub > 0);
    }
private:
    int16_t mPendSub;
    int16_t mSub;
};

class SubscribeParser
{
public:
    enum Status
    {
        Unknown,
        Subscribe,
        Psubscribe,
        Unsubscribe,
        Punsubscribe,
        Message,
        Pmessage,
        Error,
        String
    };
public:
    static Status parse(const Segment& body, int& chs);
};


#endif
