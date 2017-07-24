/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_MULTIPLEXOR_H_
#define _PREDIXY_MULTIPLEXOR_H_

#include "Socket.h"
#include "Logger.h"

class MultiplexorBase
{
public:
    enum EventEnum
    {
        ReadEvent = 0x1,
        WriteEvent = 0x2,
        ErrorEvent = 0x4,
        AddedMask = 0x8
    };
public:
    virtual ~MultiplexorBase() {}
protected:
    MultiplexorBase() {}
private:
};


#ifdef _KQUEUE_
#include "KqueueMultiplexor.h"
#elif _EPOLL_
#include "EpollMultiplexor.h"
#elif _POLL_
#include "PollMultiplexor.h"
#endif

#endif
