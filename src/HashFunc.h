/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_HASH_FUNC_H_
#define _PREDIXY_HASH_FUNC_H_

#include <string.h>
#include <stdint.h>

class Hash
{
public:
    enum Type
    {
        None,
        Atol,
        Crc16
    };
    static Hash parse(const char* str);
    static const char* hashTagStr(const char* buf, int& len, const char* tag);
    static uint16_t crc16(const char* buf, int len);
    static uint16_t crc16(uint16_t crc, char k);
    static long atol(const char* buf, int len);
public:
    Hash(Type t = None):
        mType(t)
    {
    }
    operator Type() const
    {
        return mType;
    }
    long hash(const char* buf, int len) const
    {
        switch (mType) {
        case Atol:
            return atol(buf, len);
        case Crc16:
            return crc16(buf, len);
        default:
            break;
        }
        return 0;
    }
    long hash(const char* buf, int len, const char* tag) const
    {
        buf = hashTagStr(buf, len, tag);
        return hash(buf, len);
    }
private:
    Type mType;
};


#endif
