/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_ID_H_
#define _PREDIXY_ID_H_

#include "Sync.h"
#include <vector>

template<class T>
class TID
{
public:
    TID():
        mId(++Id)
    {
    }
    long id() const
    {
        return mId;
    }
private:
    long mId;
    thread_local static long Id;
};

template<class T>
thread_local long TID<T>::Id(0);

template<class T>
class ID
{
public:
    ID():
        mId(++Id - 1)
    {
    }
    int id() const
    {
        return mId;
    }
    static int maxId()
    {
        return Id;
    }
protected:
    ~ID()
    {
    }
private:
    int mId;
    static AtomicInt Id;
};

template<class T>
AtomicInt ID<T>::Id(0);

class IDUnique
{
public:
    IDUnique(int sz = 0):
        mMarker(sz, false)
    {
    }
    void resize(int sz)
    {
        if (sz > (int)mMarker.size()) {
            mMarker.resize(sz, false);
        }
    }
    template<class C>
    int unique(C d, int num)
    {
        int n = 0;
        for (int i = 0; i < num; ++i) {
            auto p = d[i];
            if (!mMarker[p->id()]) {
                mMarker[p->id()] = true;
                d[n++] = p;
            }
        }
        for (int i = 0; i < n; ++i) {
            mMarker[d[i]->id()] = false;
        }
        return n;
    }
private:
    std::vector<bool> mMarker;
};

#endif
