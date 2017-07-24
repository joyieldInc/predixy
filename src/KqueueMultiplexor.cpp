/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "KqueueMultiplexor.h"
#include "Socket.h"

KqueueMultiplexor::KqueueMultiplexor():
    mFd(-1)
{
    int fd = kqueue();
    if (fd < 0) {
        Throw(KqueueCreateFail, "kqueue call fail %s", StrError());
    }
    mFd = fd;
}

KqueueMultiplexor::~KqueueMultiplexor()
{
    if (mFd >= 0) {
        ::close(mFd);
    }
}

bool KqueueMultiplexor::addSocket(Socket* s, int evts)
{
    return addEvent(s, evts);
}

void KqueueMultiplexor::delSocket(Socket* s)
{
    struct kevent event;
    EV_SET(&event, s->fd(), EVFILT_READ, EV_DELETE, 0, 0, s);
    kevent(mFd, &event, 1, NULL, 0, NULL);
    EV_SET(&event, s->fd(), EVFILT_WRITE, EV_DELETE, 0, 0, s);
    kevent(mFd, &event, 1, NULL, 0, NULL);
}

bool KqueueMultiplexor::addEvent(Socket* s, int evts)
{
    if ((evts & ReadEvent) && !(s->getEvent() & ReadEvent)) {
        struct kevent event;
        EV_SET(&event, s->fd(), EVFILT_READ, EV_ADD, 0, 0, s);
        int ret = kevent(mFd, &event, 1, NULL, 0, NULL);
        if (ret == -1) {
            return false;
        }
        s->setEvent(s->getEvent() | ReadEvent);
    }
    if ((evts & WriteEvent) && !(s->getEvent() & WriteEvent)) {
        struct kevent event;
        EV_SET(&event, s->fd(), EVFILT_WRITE, EV_ADD, 0, 0, s);
        int ret = kevent(mFd, &event, 1, NULL, 0, NULL);
        if (ret == -1) {
            return false;
        }
        s->setEvent(s->getEvent() | WriteEvent);
    }
    return true;

}

bool KqueueMultiplexor::delEvent(Socket* s, int evts)
{
    if ((evts & ReadEvent) && (s->getEvent() & ReadEvent)) {
        struct kevent event;
        EV_SET(&event, s->fd(), EVFILT_READ, EV_DELETE, 0, 0, s);
        int ret = kevent(mFd, &event, 1, NULL, 0, NULL);
        if (ret == -1) {
            return false;
        }
        s->setEvent(s->getEvent() & ~ReadEvent);
    }
    if ((evts & WriteEvent) && (s->getEvent() & WriteEvent)) {
        struct kevent event;
        EV_SET(&event, s->fd(), EVFILT_WRITE, EV_DELETE, 0, 0, s);
        int ret = kevent(mFd, &event, 1, NULL, 0, NULL);
        if (ret == -1) {
            return false;
        }
        s->setEvent(s->getEvent() & ~WriteEvent);
    }
    return true;
}
