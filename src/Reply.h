/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_REPLY_H_
#define _PREDIXY_REPLY_H_

class Reply
{
public:
    enum Type
    {
        None,
        Status,
        Error,
        String,
        Integer,
        Array,

        Sentinel
    };
    static const char* TypeStr[Sentinel];
};

#endif
