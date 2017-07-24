/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_RESPONSE_PARSER_H_
#define _PREDIXY_RESPONSE_PARSER_H_

#include "Predixy.h"

class ResponseParser
{
public:
    static const int MaxArrayDepth = 8;
    enum State
    {
        Idle,
        StatusHead,
        ErrorHead,
        IntegerHead,
        StringHead,
        ArrayHead,
        HeadLF,
        ArrayBody,
        StringLen,
        StringBody,
        StringBodyLF,
        LineBody,
        SubStringLen,
        SubStringLenLF,
        SubStringBody,
        SubStringBodyLF,
        SubArrayLen,
        SubArrayLenLF,
        ElementLF,
        Finished,

        Error
    };
    enum Status
    {
        Normal,
        Partial,
        Complete,
        ParseError
    };
public:
    ResponseParser();
    ~ResponseParser();
    void reset();
    Status parse(Buffer* buf, int& pos);
    bool isIdle() const
    {
        return mState == Idle;
    }
    Reply::Type type() const
    {
        return mType;
    }
    Segment& response()
    {
        return mRes;
    }
    const Segment& response() const
    {
        return mRes;
    }
    int64_t integer() const
    {
        return mInteger;
    }
private:
    Reply::Type mType;
    Segment mRes;
    State mState;
    int64_t mInteger;
    int mStringLen;
    int mStringCnt;
    int mArrayNum[MaxArrayDepth];
    int mElementCnt[MaxArrayDepth];
    int mDepth;
    bool mSign;
};

#endif
