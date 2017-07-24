/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_REQUEST_PARSER_H_
#define _PREDIXY_REQUEST_PARSER_H_

#include <map>
#include "Predixy.h"

class RequestParser
{
public:
    static const int MaxAllowInvalidByteCount = 1024;
    static const int MaxCmdLen = 32;
    enum State
    {
        Idle,        // * or inline command
        InlineBegin,
        InlineCmd,
        InlineLF,
        InlineArg,
        ArgNum,      // 2
        ArgNumLF,    // \r\n
        ArgTag,      // $           $
        ArgLen,      // 3           5
        ArgLenLF,    // \r\n        \r\n
        ArgBody,     // get         hello
        ArgBodyLF,   // \r\n        \r\n
        Finished,

        Error
    };
    enum Status
    {
        Normal,
        Partial,
        Complete,
        CmdError,
        ArgError,
        ParseError
    };
    static void init();
public:
    RequestParser();
    ~RequestParser();
    Status parse(Buffer* buf, int& pos, bool split);
    void reset();
    bool isIdle() const
    {
        return mState == Idle;
    }
    Command::Type type() const
    {
        return mType;
    }
    const Command* command() const
    {
        return mCommand;
    }
    bool isInline() const
    {
        return mInline;
    }
    Segment& request()
    {
        return mReq;
    }
    const Segment& request() const
    {
        return mReq;
    }
    int argNum() const
    {
        return mArgNum;
    }
    Segment& arg()
    {
        return mArg;
    }
    const Segment& arg() const
    {
        return mArg;
    }
    Segment& cmd()
    {
        return mCmd;
    }
    const Segment& cmd() const
    {
        return mCmd;
    }
    Segment& key()
    {
        return mKey;
    }
    const Segment& key() const
    {
        return mKey;
    }
private:
    void parseCmd();
    bool isKey(bool split) const;
private:
    Command::Type mType;
    const Command* mCommand;
                    // *2\r\n$3\r\nget\r\n$3\r\nkey\r\n
    Segment mReq;   // |------------------------------|
    Segment mCmd;   //             |-|                 
    Segment mArg;   //                    |-----------|
    Segment mKey;   //                          |-|    
    Status mStatus;
    State mState;
    bool mInline;
    int mArgNum;
    int mArgCnt;
    int mArgLen;
    int mArgBodyCnt;
    int mByteCnt;
};

#endif
