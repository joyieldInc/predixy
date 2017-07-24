/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "PollMultiplexor.h"
#include "Socket.h"

PollMultiplexor::PollMultiplexor(int maxFdSize):
    mSkts(maxFdSize),
    mFds(maxFdSize)
{
    mSkts.resize(0);
    mFds.resize(0);
}

PollMultiplexor::~PollMultiplexor()
{
}

bool PollMultiplexor::addSocket(Socket* s, int evts)
{
    if (s->fd() >= (int)mSkts.size()) {
        mSkts.resize(s->fd() + 1, {nullptr, -1});
    }
    mSkts[s->fd()].skt = s;
    mSkts[s->fd()].idx = mFds.size();
    short int e = 0;
    e |= (evts & ReadEvent) ? POLLIN : 0;
    e |= (evts & WriteEvent) ? POLLOUT : 0;
    mFds.push_back({s->fd(), e, 0});
    return true;
}

void PollMultiplexor::delSocket(Socket* s)
{
    if (s->fd() < (int)mSkts.size()) {
        int idx = mSkts[s->fd()].idx;
        if (idx >= 0 && idx < (int)mFds.size()) {
            mFds[idx] = mFds.back();
            mFds.resize(mFds.size() - 1);
            if (!mFds.empty()) {
                mSkts[mFds[idx].fd].idx = idx;
            }
        }
        mSkts[s->fd()].skt = nullptr;
        mSkts[s->fd()].idx = -1;
    }
}

bool PollMultiplexor::addEvent(Socket* s, int evts)
{
    int e = 0;
    e |= (evts & ReadEvent) ? POLLIN : 0;
    e |= (evts & WriteEvent) ? POLLOUT : 0;
    mFds[mSkts[s->fd()].idx].events |= e;
    return true;
}

bool PollMultiplexor::delEvent(Socket* s, int evts)
{
    int e = 0;
    e |= (evts & ReadEvent) ? POLLIN : 0;
    e |= (evts & WriteEvent) ? POLLOUT : 0;
    mFds[mSkts[s->fd()].idx].events &= ~e;
    return true;
}
