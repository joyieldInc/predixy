/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_ALLOC_H_
#define _PREDIXY_ALLOC_H_

#include <stdlib.h>
#include <iostream>
#include <thread>
#include "Sync.h"
#include "Exception.h"
#include "Util.h"
#include "Logger.h"
#include "Timer.h"

class AllocBase
{
public:
    DefException(MemLimit);
public:
    static void setMaxMemory(long m)
    {
        MaxMemory = m;
    }
    static long getMaxMemory()
    {
        return MaxMemory;
    }
    static long getUsedMemory()
    {
        return UsedMemory;
    }
protected:
    static long MaxMemory;
    static AtomicLong UsedMemory;
};

template<class T>
inline int allocSize()
{
    return sizeof(T);
}

template<class T, int CacheSize = 64>
class Alloc : public AllocBase
{
public:
    template<class... Targs>
    static T* create(Targs&&... args)
    {
        FuncCallTimer();
        T* obj = nullptr;
        if (Size > 0) {
            obj = Free[--Size];
        }
        if (obj) {
            try {
                new ((void*)obj) T(args...);
            } catch (...) {
                bool del = true;
                if (Size < CacheSize) {
                    Free[Size++] = obj;
                    del = false;
                }
                if (del) {
                    UsedMemory -= allocSize<T>();
                    ::operator delete((void*)obj);
                }
                throw;
            }
            logVerb("alloc create object with old memory %d @%p", allocSize<T>(), obj);
            return obj;
        }
        UsedMemory += allocSize<T>();
        if (MaxMemory == 0 || UsedMemory <= MaxMemory) {
            void* p = ::operator new(allocSize<T>(), std::nothrow);
            if (p) {
                try {
                    obj = new (p) T(args...);
                    logVerb("alloc create object with new memory %d @%p", allocSize<T>(), obj);
                    return obj;
                } catch (...) {
                    UsedMemory -= allocSize<T>();
                    ::operator delete(p);
                    throw;
                }
            } else {
                UsedMemory -= allocSize<T>();
                Throw(MemLimit, "system memory alloc fail");
            }
        } else {
            UsedMemory -= allocSize<T>();
            Throw(MemLimit, "maxmemory used");
        }
        return nullptr;
    }
    static void destroy(T* obj)
    {
        FuncCallTimer();
        bool del = true;
        obj->~T();
        if (Size < CacheSize) {
            Free[Size++] = obj;
            del = false;
        }
        if (del) {
            UsedMemory -= allocSize<T>();
            ::operator delete((void*)obj);
        }
        logVerb("alloc destroy object with size %d @%p delete %s", (int)allocSize<T>(), obj, del ? "true" : "false");
    }
private:
    thread_local static T* Free[CacheSize];
    thread_local static int Size;
};

template<class T, int CacheSize>
thread_local T* Alloc<T, CacheSize>::Free[CacheSize];
template<class T, int CacheSize>
thread_local int Alloc<T, CacheSize>::Size = 0;

template<class T, class CntType = int>
class RefCntObj
{
public:
    RefCntObj():
        mCnt(0)
    {
    }
    RefCntObj(const RefCntObj&) = delete;
    RefCntObj& operator=(const RefCntObj&) = delete;
    int count() const
    {
        return mCnt;
    }
    void ref()
    {
        FuncCallTimer();
        ++mCnt;
    }
    void unref()
    {
        int n = --mCnt;
        if (n == 0) {
            T::Allocator::destroy(static_cast<T*>(this));
        } else if (n < 0) {
            logError("unref object %p with cnt %d", this, n);
            abort();
        }
    }
protected:
    ~RefCntObj()
    {
        mCnt = 0;
    }
private:
    CntType mCnt;
};

template<class T>
class SharePtr
{
public:
    SharePtr():
        mObj(nullptr)
    {
    }
    SharePtr(T* obj):
        mObj(obj)
    {
        if (obj) {
            obj->ref();
        }
    }
    SharePtr(const SharePtr<T>& sp):
        mObj(sp.mObj)
    {
        if (mObj) {
            mObj->ref();
        }
    }
    SharePtr(SharePtr<T>&& sp):
        mObj(sp.mObj)
    {
        sp.mObj = nullptr;
    }
    ~SharePtr()
    {
        if (mObj) {
            mObj->unref();
            mObj = nullptr;
        }
    }
    operator T*() const
    {
        return mObj;
    }
    SharePtr& operator=(const SharePtr& sp)
    {
        if (this != &sp) {
            T* obj = mObj;
            mObj = sp.mObj;
            if (mObj) {
                mObj->ref();
            }
            if (obj) {
                obj->unref();
            }
        }
        return *this;
    }
    T& operator*()
    {
        return *mObj;
    }
    const T& operator*() const
    {
        return *mObj;
    }
    T* operator->()
    {
        return mObj;
    }
    const T* operator->() const
    {
        return mObj;
    }
    bool operator!() const
    {
        return !mObj;
    }
    operator bool() const
    {
        return mObj;
    }
    bool operator==(const SharePtr& sp) const
    {
        return mObj == sp.mObj;
    }
    bool operator!=(const SharePtr& sp) const
    {
        return mObj != sp.mObj;
    }
private:
    T* mObj;
};

#endif
