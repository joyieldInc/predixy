/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_TRANSACTION_H_
#define _PREDIXY_TRANSACTION_H_

#include <stdint.h>

class Transaction
{
public:
    static const int16_t MaxWatch = 4096;
    static const int16_t MaxMulti = 4096;
public:
    Transaction():
        mPendWatch(0),
        mWatch(0),
        mPendMulti(0),
        mMulti(0)
    {
    }
    bool inPendWatch() const
    {
        return mPendWatch > 0;
    }
    int incrPendWatch()
    {
        return mPendWatch < MaxWatch ? ++mPendWatch : 0;
    }
    int decrPendWatch(int cnt = 1)
    {
        if (mPendWatch > cnt) {
            mPendWatch -= cnt;
        } else {
            mPendWatch = 0;
        }
        return mPendWatch;
    }
    bool inWatch() const
    {
        return mWatch > 0;
    }
    int incrWatch()
    {
        return mWatch < MaxWatch ? ++mWatch : 0;
    }
    int decrWatch()
    {
        decrPendWatch();
        return mWatch > 0 ? --mWatch : 0;
    }
    void unwatch()
    {
        decrPendWatch(mWatch);
        mWatch = 0;
    }
    bool inPendMulti() const
    {
        return mPendMulti > 0;
    }
    int incrPendMulti()
    {
        return mPendMulti < MaxMulti ? ++mPendMulti : 0;
    }
    int decrPendMulti()
    {
        return mPendMulti > 0 ? --mPendMulti : 0;
    }
    bool inMulti() const
    {
        return mMulti > 0;
    }
    int incrMulti()
    {
        return mMulti < MaxMulti ? ++mMulti : 0;
    }
    int decrMulti()
    {
        decrPendMulti();
        return mMulti > 0 ? --mMulti : 0;
    }
    bool inTransaction() const
    {
        return mPendWatch > 0 || mPendMulti > 0;
    }
private:
    int16_t mPendWatch;
    int16_t mWatch;
    int16_t mPendMulti;
    int16_t mMulti;
};

#endif
