/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_KQUEUE_MULTIPLEXOR_H_
#define _PREDIXY_KQUEUE_MULTIPLEXOR_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "Multiplexor.h"
#include "Util.h"

class KqueueMultiplexor : public MultiplexorBase
{
public:
    DefException(KqueueCreateFail);
    DefException(KqueueWaitFail);
    DefException(AddSocketFail);
public:
    KqueueMultiplexor();
    ~KqueueMultiplexor();
    bool addSocket(Socket* s, int evts = ReadEvent);
    void delSocket(Socket* s);
    bool addEvent(Socket* s, int evts);
    bool delEvent(Socket* s, int evts);
    template<class T>
    int wait(long usec, T* handler);
private:
    int mFd;
    static const int MaxEvents = 1024;
    struct kevent mEvents[MaxEvents];
};


template<class T>
int KqueueMultiplexor::wait(long usec, T* handler)
{
    struct timespec timeout;
    timeout.tv_sec = usec / 1000000;
    timeout.tv_nsec = (usec % 1000000) * 1000;
    int num = kevent(mFd, nullptr, 0, mEvents, MaxEvents, usec < 0 ? nullptr : &timeout);
    if (num == -1) {
        if (errno == EINTR) {
            return 0;
        }
        Throw(KqueueWaitFail, "h %d kqueue wait fail %s",
              handler->id(), StrError());
    }
    for (int i = 0; i < num; ++i) {
        Socket* s = static_cast<Socket*>(mEvents[i].udata);
        int evts = 0;
        if (mEvents[i].flags & EV_ERROR) {
            evts = ErrorEvent;
        } else if (mEvents[i].filter == EVFILT_READ) {
            evts = ReadEvent;
        } else if (mEvents[i].filter == EVFILT_WRITE) {
            evts = WriteEvent;
        }
        handler->handleEvent(s, evts);
    }
    return num;
}


typedef KqueueMultiplexor Multiplexor;

#endif
