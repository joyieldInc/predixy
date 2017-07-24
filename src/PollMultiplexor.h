/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_POLL_MULTIPLEXOR_H_
#define _PREDIXY_POLL_MULTIPLEXOR_H_

#include <unistd.h>
#include <poll.h>
#include <vector>
#include "Multiplexor.h"
#include "Util.h"

class PollMultiplexor : public MultiplexorBase
{
public:
    DefException(PollWaitFail);
public:
    PollMultiplexor(int maxFdSize = 81920);
    ~PollMultiplexor();
    bool addSocket(Socket* s, int evts = ReadEvent);
    void delSocket(Socket* s);
    bool addEvent(Socket* s, int evts);
    bool delEvent(Socket* s, int evts);
    template<class T>
    int wait(long usec, T* handler);
private:
    struct SktIdx
    {
        Socket* skt;
        int idx;
    };
private:
    std::vector<SktIdx> mSkts;
    std::vector<struct pollfd> mFds;
};


template<class T>
int PollMultiplexor::wait(long usec, T* handler)
{
    int timeout = usec < 0 ? usec : usec / 1000;
    logVerb("poll wait with timeout %ld usec", usec);
    int num = poll(mFds.data(), mFds.size(), timeout);
    logVerb("poll wait return with events:%d", num);
    if (num == -1) {
        if (errno == EINTR) {
            return 0;
        }
        Throw(PollWaitFail, "pool wait fail %s", StrError());
    }
    int cnt = 0;
    for (auto e : mFds) {
        if (e.revents) {
            Socket* s = mSkts[e.fd].skt;
            int evts = 0;
            evts |= (e.revents & POLLIN) ? ReadEvent : 0;
            evts |= (e.revents & POLLOUT) ? WriteEvent : 0;
            evts |= (e.revents & (POLLERR|POLLHUP)) ? ErrorEvent : 0;
            handler->handleEvent(s, evts);
            if (++cnt == num) {
                break;
            }
        }
    }
    return num;
}


typedef PollMultiplexor Multiplexor;

#endif
