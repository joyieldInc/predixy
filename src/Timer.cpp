/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <stdio.h>
#include <algorithm>
#include <iostream>
#include "Timer.h"

Mutex TimerPoint::Mtx;
TimerPoint* TimerPoint::Head = nullptr;
TimerPoint* TimerPoint::Tail = nullptr;
int TimerPoint::PointCnt = 0;

TimerPoint::TimerPoint(const char* key):
    mKey(key),
    mElapsed(0),
    mCount(0),
    mNext(nullptr)
{
    UniqueLock<Mutex> lck(Mtx);
    ++PointCnt;
    if (Tail) {
        Tail->mNext = this;
        Tail = this;
    } else {
        Head = Tail = this;
    }
}

#define StaticPointLen 1024
void TimerPoint::report()
{
    TimerPoint* points0[StaticPointLen];
    TimerPoint** points = points0;
    int cnt = PointCnt;
    if (cnt <= 0) {
        return;
    } else if (cnt > StaticPointLen) {
        points = new TimerPoint*[cnt];
    }
    int i = 0;
    for (TimerPoint* p = Head; p && i < cnt; p = p->mNext) {
        points[i++] = p;
    }
    std::sort(points, points + cnt,
            [](TimerPoint* p1, TimerPoint* p2)
            {return p1->elapsed() > p2->elapsed();});
        printf("%16s %12s %8s %s\n","Total(us)", "Count", "Avg(ns)", "Point" );
    for (i = 0; i < cnt; ++i) {
        auto p = points[i];
        printf("%16ld %12ld %9ld %s\n",
                p->elapsed(), p->count(),
                p->elapsed()*1000/p->count(), p->key());
    }
    if (points != points0) {
        delete[] points;
    }
}

