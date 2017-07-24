/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SYNC_H_
#define _PREDIXY_SYNC_H_

#ifndef _PREDIXY_SINGLE_THREAD_

#include <atomic>
#include <mutex>

#define Atomic              std::atomic
#define AtomicInt           std::atomic<int>
#define AtomicLong          std::atomic<long>
#define AtomicCAS(v, o, n)  v.compare_exchange_strong(o, n)
#define Mutex               std::mutex
#define UniqueLock          std::unique_lock
#define TryLockTag          std::try_to_lock_t()


#else //_PREDIXY_SINGLE_THREAD_

#define AtomicInt           int
#define AtomicLong          long
#define AtomicCAS(v, oldVal, newVal) (v == (oldVal) ? v = (newVal), true : false)
#define TryLockTag          ""

class Mutex
{
public:
    void lock() const
    {
    }
    void unlock() const
    {
    }
};

template<class M>
class UniqueLock
{
public:
    template<class... A>
    UniqueLock(A&&... args)
    {
    }
    operator bool() const
    {
        return true;
    }
};


#endif //_PREDIXY_MULTI_THREAD_


#endif
