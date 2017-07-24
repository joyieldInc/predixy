/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_DISTRIBUTION_H_
#define _PREDIXY_DISTRIBUTION_H_

class Distribution
{
public:
    enum Type
    {
        None,
        Modula,
        Random
    };
public:
    Distribution(Type t = None):
        mType(t)
    {
    }
    operator Type() const
    {
        return mType;
    }
    const char* name() const;
    static Distribution parse(const char* str);
private:
    Type mType;
};

#endif
