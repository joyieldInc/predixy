/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "Connection.h"

Connection::Connection():
    mPostEvts(0),
    mBufCnt(0),
    mDb(0),
    mCloseASAP(false)
{
}

BufferPtr Connection::getBuffer(Handler* h, bool allowNew)
{
    FuncCallTimer();
    if (allowNew && mBuf) {
        if (mBufCnt >= Const::MaxBufListNodeNum) {
            mBuf = nullptr;
            mBufCnt = 0;
        } else if (mBuf->room() < Const::MinBufSpaceLeft) {
            mBuf = nullptr;
            mBufCnt = 0;
        }
    }
    if (!mBuf || mBuf->full()) {
        BufferPtr buf = BufferAlloc::create();
        if (mBuf) {
            mBuf->concat(buf);
        }
        mBuf = buf;
        ++mBufCnt;
    }
    return mBuf;
}

