/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CONNECTION_H_
#define _PREDIXY_CONNECTION_H_

#include "Socket.h"
#include "List.h"
#include "Common.h"
#include "Buffer.h"

class Handler;

class Connection
{
public:
    enum ClassType
    {
        AcceptType = Socket::CustomType,
        ConnectType
    };
    enum StatusEnum
    {
        ParseError = Socket::CustomStatus,
        LogicError,
        TimeoutError
    };
public:
    Connection();
    void closeASAP()
    {
        mCloseASAP = true;
    }
    bool isCloseASAP() const
    {
        return mCloseASAP;
    }
    int getPostEvent() const
    {
        return mPostEvts;
    }
    void addPostEvent(int evts)
    {
        mPostEvts |= evts;
    }
    void setPostEvent(int evts)
    {
        mPostEvts = evts;
    }
    int db() const
    {
        return mDb;
    }
    void setDb(int db)
    {
        mDb = db;
    }
protected:
    BufferPtr getBuffer(Handler* h, bool allowNew);
private:
    int mPostEvts;
    BufferPtr mBuf;
    int mBufCnt;
    int mDb;
    bool mCloseASAP;
};

#endif
