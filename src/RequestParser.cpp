/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "RequestParser.h"

RequestParser::RequestParser()
{
    reset();
}

RequestParser::~RequestParser()
{
    reset();
}

void RequestParser::reset()
{
    mType = Command::None;
    mCommand = nullptr;
    mReq.clear();
    mCmd.clear();
    mArg.clear();
    mKey.clear();
    mStatus = Normal;
    mState = Idle;
    mInline = false;
    mArgNum = 0;
    mArgCnt = 0;
    mArgLen = 0;
    mArgBodyCnt = 0;
    mByteCnt = 0;
}

bool RequestParser::isKey(bool split) const
{
    if (mCommand) {
        switch (mCommand->mode & Command::KeyMask) {
        case Command::NoKey:
            return false;
        case Command::MultiKey:
            return split ? mArgCnt > 0 : mArgCnt == 1;
        case Command::SMultiKey:
            return mArgCnt > 0;
        case Command::MultiKeyVal:
            return split ? (mArgCnt & 1) : mArgCnt == 1;
        case Command::KeyAt3:
            return mArgCnt == 3;
        default:
            return mArgCnt == 1;
        }
    }
    return false;
}

RequestParser::Status RequestParser::parse(Buffer* buf, int& pos, bool split)
{
    FuncCallTimer();
    int start = pos;
    char* cursor = buf->data() + pos;
    char* end = buf->tail();
    int error = 0;
    while (cursor < end && !error) {
        ++mByteCnt;
        if (mStatus != Normal && mByteCnt > MaxAllowInvalidByteCount) {
            logWarn("request command argument invalid");
            error = __LINE__;
            break;
        }
        char ch = *cursor;
        switch (mState) {
        case Idle:
            mArgNum = 0;
            if (ch == '*') {
                mReq.begin().buf = buf;
                mReq.begin().pos = pos;
                mState = ArgNum;
                break;
            } else {
                mState = InlineBegin;
                mInline = true;
            }
        case InlineBegin:
            if (ch == '\r') {
                error = __LINE__;
            } else if (!isspace(ch)) {
                mReq.begin().buf = buf;
                mReq.begin().pos = pos;
                mCmd.begin() = mReq.begin();
                mState = InlineCmd;
            }
            break;
        case InlineCmd:
            if (isspace(ch)) {
                mCmd.end().buf = buf;
                mCmd.end().pos = pos;
                parseCmd();
                mState = ch == '\r' ? InlineLF : InlineArg;
            }
            break;
        case InlineArg:
            if (ch == '\r') {
                mState = InlineLF;
            }
            break;
        case InlineLF:
            if (ch == '\n') {
                mState = Finished;
                goto Done;
            } else {
                error = __LINE__;
            }
            break;
        case ArgNum:
            if (ch >= '0' && ch <= '9') {
                mArgNum = mArgNum * 10 + (ch - '0');
            } else if (ch == '\r') {
                //mState = mArgNum > 0 ? ArgNumLF : Error;
                mArgNum > 0 ? mState = ArgNumLF : error = __LINE__;
            } else {
                error = __LINE__;
            }
            break;
        case ArgNumLF:
            mArgCnt = 0;
            //mState = ch == '\n' ? ArgTag : Error;
            ch == '\n' ? mState = ArgTag : error = __LINE__;
            break;
        case ArgTag:
            mArgLen = 0;
            if (ch == '$') {
                mState = ArgLen;
                if (isKey(split)) {
                    mArg.begin().buf = buf;
                    mArg.begin().pos = pos;
                }
            } else {
                error = __LINE__;
            }
            break;
        case ArgLen:
            if (ch >= '0' && ch <= '9') {
                mArgLen = mArgLen * 10 + (ch - '0');
            } else if (ch == '\r') {
                //mState = mArgLen >= 0 ? ArgLenLF : Error;
                mArgLen >= 0 ? mState = ArgLenLF : error = __LINE__;
            } else {
                error = __LINE__;
            }
            break;
        case ArgLenLF:
            mArgBodyCnt = 0;
            ch == '\n' ? mState = ArgBody : error = __LINE__;
            break;
        case ArgBody:
            if (mArgBodyCnt == 0) {
                if (mArgCnt == 0) {
                    mCmd.begin().buf = buf;
                    mCmd.begin().pos = pos;
                } else if (isKey(split)) {
                    mKey.begin().buf = buf;
                    mKey.begin().pos = pos;
                }
            }
            if (mArgBodyCnt + (end - cursor) > mArgLen) {
                pos += mArgLen - mArgBodyCnt;
                cursor = buf->data() + pos;
                if (*cursor == '\r') {
                    mState = ArgBodyLF;
                    if (mArgCnt == 0) {
                        mCmd.end().buf = buf;
                        mCmd.end().pos = pos;
                        parseCmd();
                    } else if (isKey(split)) {
                        mKey.end().buf = buf;
                        mKey.end().pos = pos;
                    }
                } else {
                    error = __LINE__;
                }
            } else {
                mArgBodyCnt += end - cursor;
                pos = buf->length() - 1;
            }
            break;
        case ArgBodyLF:
            if (ch == '\n') {
                if (++mArgCnt == mArgNum) {
                    mState = Finished;
                    goto Done;
                } else {
                    mState = ArgTag;
                    if (mArgCnt > 1 && isKey(split) && mStatus == Normal &&
                        (mCommand->mode&(Command::MultiKey|Command::SMultiKey|Command::MultiKeyVal))) {
                        goto Done;
                    }
                    if (mArgCnt > 1 && mCommand && mStatus == Normal && split) {
                        if (mCommand->isMultiKey()) {
                            goto Done;
                        } else if (mCommand->isMultiKeyVal() && (mArgCnt & 1)) {
                            goto Done;
                        }
                    }
                }
            } else {
                error = __LINE__;
            }
            break;
        default:
            error = __LINE__;
            break;
        }
        ++pos;
        cursor = buf->data() + pos;
    }
    if (error) {
        SString<64> bufHex;
        bufHex.printHex(buf->data() + start, buf->length() - start);
        SString<16> errHex;
        errHex.printHex(cursor - 1, end - cursor + 1);
        logDebug("request parse error %d state %d buf:%s errpos %d err:%s",
                error, mState, bufHex.data(),
                pos - 1 - start, errHex.data());
        return ParseError;
    }
    return Normal;
Done:
    mReq.end().buf = buf;
    mReq.end().pos = ++pos;
    mReq.rewind();
    mArg.end() = mReq.end();
    mArg.rewind();
    if (mState == Finished) {
        return mStatus == Normal ? Complete : mStatus;
    } else {
        return mStatus == Normal ? Partial : ParseError;
    }
}

void RequestParser::parseCmd()
{
    FuncCallTimer();
    SegmentStr<MaxCmdLen> cmd(mCmd);
    if (!cmd.complete()) {
        mStatus = CmdError;
        mType = Command::None;
        logNotice("unknown request cmd too long:%.*s...",
                cmd.length(), cmd.data());
        return;
    }
    auto c = Command::find(cmd);
    if (!c) {
        mStatus = CmdError;
        mType = Command::None;
        logNotice("unknown request cmd:%.*s", cmd.length(), cmd.data());
        return;
    }
    mType = c->type;
    mCommand = c;
    if (mInline) {
        if (mType != Command::Ping) {
            mStatus = CmdError;
            logNotice("unsupport command %s in inline command protocol", c->name);
            return;
        }
        return;
    }
    if (mArgNum < c->minArgs || mArgNum > c->maxArgs) {
        mStatus = ArgError;
        logNotice("request argument is invalid cmd %.*s argnum %d",
                cmd.length(), cmd.data(), mArgNum);
        return;
    }
    switch (mType) {
    case Command::Mset:
    case Command::Msetnx:
        if (!(mArgNum & 1)) {
            mStatus = ArgError;
            logNotice("request argument is invalid cmd %.*s argnum %d",
                    cmd.length(), cmd.data(), mArgNum);
            return;
        }
        break;
    default:
        break;
    }
}

