/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <string.h>
#include <strings.h>
#include "HashFunc.h"


Hash Hash::parse(const char* str)
{
    if (strcasecmp(str, "atol") == 0) {
        return Atol;
    } else if (strcasecmp(str, "crc16") == 0) {
        return Crc16;
    }
    return None;
}

const char* Hash::hashTagStr(const char* buf, int& len, const char* tag)
{
    if (tag && tag[0] && tag[1]) {
        int i = 0;
        while (i < len && buf[i] != tag[0]) {
            ++i;
        }
        if (i == len) {
            return buf;
        }
        const char* b = buf + ++i;
        while (i < len && buf[i] != tag[1]) {
            ++i;
        }
        if (i == len) {
            return buf;
        }
        const char* e = buf + i;
        if (b < e) {
            len = e - b;
            return b;
        }
    }
    return buf;
}

long Hash::atol(const char* buf, int len)
{
    long v = 0;
    if (buf) {
        int i = 0;
        if (buf[i] == '+') {
            ++i;
        } else if (buf[i] == '-') {
            ++i;
        }
        for ( ; i < len; ++i) {
            if (buf[i] >= '0' && buf[i] <= '9') {
                v = v * 10 + buf[i] - '0';
            } else {
                break;
            }
        }
        if (buf[0] == '-') {
            v = -v;
        }
    }
    return v;
}
