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
        // Parse count from the end of the response
        // Format: *3\r\n$N\r\nsubscribe\r\n$M\r\n<channel>\r\n:<count>\r\n
        // The count is always the last integer after a colon
        int len = body.length();
        // Look at the tail, but use a larger buffer for long channel names
        int tailLen = len < 256 ? len : 256;
        Segment tmp(body);
        tmp.rewind();
        if (len > tailLen) {
            tmp.use(len - tailLen);
        }
        char buf[256 + 1];
        int n = tmp.dump(buf, tailLen);
        buf[n] = '\0';
        // Search backwards for colon followed by digits
        const char* p = buf + n;
        while (p > buf) {
            if (*--p == ':') {
                // Found colon, parse the number after it
                const char* numStart = p + 1;
                // Skip whitespace
                while (*numStart == ' ' || *numStart == '\t') {
                    numStart++;
                }
                if (*numStart >= '0' && *numStart <= '9') {
                    chs = atoi(numStart);
                    break;
                }
            }
        }
    }
    return st;
}
