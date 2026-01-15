/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Subscribe.h"

Subscribe::Subscribe():
    mPendSub(0),
    mSub(0)
{
}



SubscribeParser::Status SubscribeParser::parse(const Segment& body, int& chs)
{
    SegmentStr<128> str(body);
    Status st = Unknown;
    chs = 0;
    if (str.hasPrefix("*3\r\n$7\r\nmessage\r\n")) {
        st = Message;
    } else if (str.hasPrefix("*4\r\n$8\r\npmessage\r\n")) {
        st = Pmessage;
    } else if (str.hasPrefix("*3\r\n$9\r\nsubscribe\r\n")) {
        st = Subscribe;
        chs = -1;
    } else if (str.hasPrefix("*3\r\n$10\r\npsubscribe\r\n")) {
        st = Psubscribe;
        chs = -1;
    } else if (str.hasPrefix("*3\r\n$11\r\nunsubscribe\r\n")) {
        st = Unsubscribe;
        chs = -1;
    } else if (str.hasPrefix("*3\r\n$12\r\npunsubscribe\r\n")) {
        st = Punsubscribe;
        chs = -1;
    } else if (str.hasPrefix("-")) {
        st = Error;
    } else if (str.hasPrefix("$")) {
        st = String;
    }
    if (chs < 0) {
        int len = body.length();
        int tailLen = len < 64 ? len : 64;
        Segment tmp(body);
        tmp.rewind();
        if (len > tailLen) {
            tmp.use(len - tailLen);
        }
        char buf[64 + 1];
        int n = tmp.dump(buf, tailLen);
        buf[n] = '\0';
        const char* p = buf + n;
        while (p > buf) {
            if (*--p == ':') {
                chs = atoi(p + 1);
                break;
            }
        }
    }
    return st;
}
