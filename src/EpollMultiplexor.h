/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_EPOLL_MULTIPLEXOR_H_
#define _PREDIXY_EPOLL_MULTIPLEXOR_H_

#include <unistd.h>
#include <sys/epoll.h>
#include "Multiplexor.h"
#include "Exception.h"
#include "Util.h"

class EpollMultiplexor : public MultiplexorBase
{
public:
    DefException(EpollCreateFail);
    DefException(EpollWaitFail);
public:
    EpollMultiplexor();
    ~EpollMultiplexor();
    bool addSocket(Socket* s, int evts = ReadEvent);
    void delSocket(Socket* s);
    bool addEvent(Socket* s, int evts);
    bool delEvent(Socket* s, int evts);
    template<class T>
    int wait(long usec, T* handler);
private:
    int mFd;
    static const int MaxEvents = 1024;
    epoll_event mEvents[MaxEvents];
};


template<class T>
int EpollMultiplexor::wait(long usec, T* handler)
{
    int timeout = usec < 0 ? usec : usec / 1000;
    logVerb("epoll wait with timeout %ld usec", usec);
    int num = epoll_wait(mFd, mEvents, MaxEvents, timeout);
    logVerb("epoll wait return with event count:%d", num);
    if (num == -1) {
        if (errno == EINTR) {
            return 0;
        }
        Throw(EpollWaitFail, "handler %d epoll wait fail %s", handler->id(), StrError());
    }
    for (int i = 0; i < num; ++i) {
        Socket* s = static_cast<Socket*>(mEvents[i].data.ptr);
        int evts = 0;
        evts |= (mEvents[i].events & EPOLLIN) ? ReadEvent : 0;
        evts |= (mEvents[i].events & EPOLLOUT) ? WriteEvent : 0;
        evts |= (mEvents[i].events & (EPOLLERR|EPOLLHUP)) ? ErrorEvent : 0;
        handler->handleEvent(s, evts);
    }
    return num;
}


typedef EpollMultiplexor Multiplexor;
#define _MULTIPLEXOR_ASYNC_ASSIGN_

#endif
