/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "EpollMultiplexor.h"
#include "Socket.h"

EpollMultiplexor::EpollMultiplexor():
    mFd(-1)
{
    int fd = epoll_create(1024);
    if (fd < 0) {
        Throw(EpollCreateFail, "epoll create fail %s", StrError());
    }
    mFd = fd;
}

EpollMultiplexor::~EpollMultiplexor()
{
    if (mFd >= 0) {
        ::close(mFd);
    }
}

bool EpollMultiplexor::addSocket(Socket* s, int evts)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events |= (evts & ReadEvent) ? EPOLLIN : 0;
    event.events |= (evts & WriteEvent) ? EPOLLOUT : 0;
    event.events |= EPOLLET;
    event.data.ptr = s;
    s->setEvent(evts);
    int ret = epoll_ctl(mFd, EPOLL_CTL_ADD, s->fd(), &event);
    return ret == 0;
}

void EpollMultiplexor::delSocket(Socket* s)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    epoll_ctl(mFd, EPOLL_CTL_DEL, s->fd(), &event);
    s->setEvent(0);
}

bool EpollMultiplexor::addEvent(Socket* s, int evts)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = s;
    if ((evts & ReadEvent) || (s->getEvent() & ReadEvent)) {
        event.events |= EPOLLIN;
    }
    if ((evts & WriteEvent) || (s->getEvent() & WriteEvent)) {
        event.events |= EPOLLOUT;
    }
    if ((s->getEvent() | evts) != s->getEvent()) {
        event.events |= EPOLLET;
        int ret = epoll_ctl(mFd, EPOLL_CTL_MOD, s->fd(), &event);
        if (ret == 0) {
            s->setEvent(s->getEvent() | evts);
        } else {
            return false;
        }
    }
    return true;
}

bool EpollMultiplexor::delEvent(Socket* s, int evts)
{
    bool mod = false;
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = s;
    if (s->getEvent() & ReadEvent) {
        if (evts & ReadEvent) {
            mod = true;
        } else {
            event.events |= EPOLLIN;
        }
    }
    if (s->getEvent() & WriteEvent) {
        if (evts & WriteEvent) {
            mod = true;
        } else {
            event.events |= EPOLLOUT;
        }
    }
    if (mod) {
        event.events |= EPOLLET;
        //event.events |= EPOLLONESHOT;
        int ret = epoll_ctl(mFd, EPOLL_CTL_MOD, s->fd(), &event);
        if (ret == 0) {
            s->setEvent(s->getEvent() & ~evts);
        } else {
            return false;
        }
    }
    return true;
}
