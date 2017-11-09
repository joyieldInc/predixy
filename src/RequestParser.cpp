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
    mKey.clear();
    mStatus = Normal;
    mState = Idle;
    mInline = false;
    mEscape = false;
    mQuote = '\0';
    mArgNum = 0;
    mArgCnt = 0;
    mArgLen = 0;
    mArgBodyCnt = 0;
    mByteCnt = 0;
}

inline bool RequestParser::isKey(bool split) const
{
    switch (mCommand->mode & Command::KeyMask) {
    case Command::NoKey:
        return false;
    case Command::MultiKey:
        return split ? mArgCnt > 0 : mArgCnt == 1;
    case Command::SMultiKey:
        return mArgCnt > 0;
    case Command::MultiKeyVal:
        return split ? (mArgCnt & 1) : mArgCnt == 1;
    case Command::KeyAt2:
        return mArgCnt == 2;
    case Command::KeyAt3:
        return mArgCnt == 3;
    default:
        return mArgCnt == 1;
    }
    return false;
}

inline bool RequestParser::isSplit(bool split) const
{
    if (mCommand->mode & Command::MultiKey) {
        return split && mStatus == Normal && mArgNum > 2 && isKey(true);
    } else if (mCommand->mode & Command::MultiKeyVal) {
        return split && mStatus == Normal && mArgNum > 3 && isKey(true);
    } else if (mCommand->mode & Command::SMultiKey) {
        return mStatus == Normal && mArgNum > 2;
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
            /* NO break */
        case InlineBegin:
            if (ch == '\n') {
                mState = Idle;
                //error = __LINE__;
            } else if (!isspace(ch)) {
                mReq.begin().buf = buf;
                mReq.begin().pos = pos;
                mCmd[0] = tolower(ch);
                mArgLen = 1;
                mState = InlineCmd;
            }
            break;
        case InlineCmd:
            if (isspace(ch)) {
                mCmd[mArgLen < Const::MaxCmdLen ? mArgLen : Const::MaxCmdLen - 1] = '\0';
                parseCmd();
                mArgCnt = 1;
                if (ch == '\n') {
                    mArgNum = 1;
                    mState = Finished;
                    goto Done;
                }
                mState = InlineArgBegin;
            } else {
                if (mArgLen < Const::MaxCmdLen) {
                    mCmd[mArgLen] = tolower(ch);
                }
                ++mArgLen;
            }
            break;
        case InlineArgBegin:
            if (ch == '\n') {
                mArgNum = mArgCnt;
                mState = Finished;
                goto Done;
            } else if (isspace(ch)) {
                break;
            }
            if (mArgCnt == 1) {
                mKey.begin().buf = buf;
                mKey.begin().pos = pos;
            }
            mState = InlineArg;
            /* NO break */
        case InlineArg:
            if (mEscape) {
                mEscape = false;
            } else if (mQuote) {
                if (ch == mQuote) {
                    mState = InlineArgEnd;
                } else if (ch == '\\') {
                    mEscape = true;
                } else if (ch == '\n') {
                    error = __LINE__;
                }
            } else {
                if (isspace(ch)) {
                    if (mArgCnt == 1) {
                        mKey.end().buf = buf;
                        mKey.end().pos = pos;
                    }
                    ++mArgCnt;
                    if (ch == '\n') {
                        mArgNum = mArgCnt;
                        mState = Finished;
                        goto Done;
                    } else {
                        mState = InlineArgBegin;
                    }
                } else if (ch == '\'' || ch == '"') {
                    mQuote = ch;
                }
            }
            break;
        case InlineArgEnd:
            if (isspace(ch)) {
                if (mArgCnt == 1) {
                    mKey.end().buf = buf;
                    mKey.end().pos = pos;
                }
                ++mArgCnt;
                if (ch == '\n') {
                    mArgNum = mArgCnt;
                    mState = Finished;
                    goto Done;
                } else {
                    mState = InlineArgBegin;
                }
            } else {
                error = __LINE__;
            }
            break;
        case ArgNum:
            if (ch >= '0' && ch <= '9') {
                mArgNum = mArgNum * 10 + (ch - '0');
            } else if (ch == '\r') {
                mArgNum > 0 ? mState = ArgNumLF : error = __LINE__;
            } else {
                error = __LINE__;
            }
            break;
        case ArgNumLF:
            mArgCnt = 0;
            ch == '\n' ? mState = CmdTag : error = __LINE__;
            break;
        case CmdTag:
            if (ch == '$') {
                mArgLen = 0;
                mState = CmdLen;
            } else {
                error = __LINE__;
            }
            break;
        case CmdLen:
            if (ch >= '0' && ch <= '9') {
                mArgLen = mArgLen * 10 + (ch - '0');
            } else if (ch == '\r') {
                mArgLen > 0 ? mState = CmdLenLF : error = __LINE__;
            } else {
                error = __LINE__;
            }
            break;
        case CmdLenLF:
            if (ch == '\n') {
                mArgBodyCnt = 0;
                mState = mArgLen < Const::MaxCmdLen ? CmdBody : CmdBodyTooLong;
            } else {
                error = __LINE__;
            }
            break;
        case CmdBody:
            if (mArgBodyCnt == mArgLen) {
                mCmd[mArgLen] = '\0';
                ch == '\r' ? mState = CmdBodyLF : error = __LINE__;
            } else {
                mCmd[mArgBodyCnt++] = tolower(ch);
            }
            break;
        case CmdBodyTooLong:
            if (mArgBodyCnt == mArgLen) {
                mCmd[Const::MaxCmdLen - 1] = '\0';
                ch == '\r' ? mState = CmdBodyLF : error = __LINE__;
            } else {
                if (mArgBodyCnt < Const::MaxCmdLen) {
                    mCmd[mArgBodyCnt] = ch;
                }
                ++mArgBodyCnt;
            }
            break;
        case CmdBodyLF:
            if (ch == '\n') {
                parseCmd();
                if (++mArgCnt == mArgNum) {
                    mState = Finished;
                    goto Done;
                }
                if (mCommand->mode & Command::KeyMask) {
                    mState = SArgTag;
                } else {
                    mState = KeyTag;
                }
            } else {
                error = __LINE__;
            }
            break;
        case KeyTag:
            mArgLen = 0;
            ch == '$' ? mState = KeyLen : error = __LINE__;
            break;
        case KeyLen:
            if (ch >= '0' && ch <= '9') {
                mArgLen = mArgLen * 10 + (ch - '0');
            } else if (ch == '\r') {
                mArgLen >= 0 ? mState = KeyLenLF : error = __LINE__;
            } else {
                error = __LINE__;
            }
            break;
        case KeyLenLF:
            if (ch == '\n') {
                mArgBodyCnt = 0;
                mState = KeyBody;
            } else {
                error = __LINE__;
            }
            break;
        case KeyBody:
            if (mArgBodyCnt == 0) {
                mKey.begin().buf = buf;
                mKey.begin().pos = pos;
            }
            if (mArgBodyCnt + (end - cursor) > mArgLen) {
                pos += mArgLen - mArgBodyCnt;
                cursor = buf->data() + pos;
                if (*cursor == '\r') {
                    mState = KeyBodyLF;
                    mKey.end().buf = buf;
                    mKey.end().pos = pos;
                } else {
                    error = __LINE__;
                }
            } else {
                mArgBodyCnt += end - cursor;
                pos = buf->length() - 1;
            }
            break;
        case KeyBodyLF:
            if (ch == '\n') {
                if (++mArgCnt == mArgNum) {
                    mState = Finished;
                    goto Done;
                }
                mState = ArgTag;
            } else {
                error = __LINE__;
            }
            break;
        case ArgTag:
            mArgLen = 0;
            ch == '$' ? mState = ArgLen : error = __LINE__;
            break;
        case ArgLen:
            if (ch >= '0' && ch <= '9') {
                mArgLen = mArgLen * 10 + (ch - '0');
            } else if (ch == '\r') {
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
            if (mArgBodyCnt + (end - cursor) > mArgLen) {
                pos += mArgLen - mArgBodyCnt;
                cursor = buf->data() + pos;
                *cursor == '\r' ? mState = ArgBodyLF : error = __LINE__;
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
                }
            } else {
                error = __LINE__;
            }
            break;
        case SArgTag:
            mArgLen = 0;
            if (ch == '$') {
                mState = SArgLen;
                if (isSplit(split)) {
                    mReq.begin().buf = buf;
                    mReq.begin().pos = pos;
                }
            } else {
                error = __LINE__;
            }
            break;
        case SArgLen:
            if (ch >= '0' && ch <= '9') {
                mArgLen = mArgLen * 10 + (ch - '0');
            } else if (ch == '\r') {
                mArgLen >= 0 ? mState = SArgLenLF : error = __LINE__;
            } else {
                error = __LINE__;
            }
            break;
        case SArgLenLF:
            mArgBodyCnt = 0;
            ch == '\n' ? mState = SArgBody : error = __LINE__;
            break;
        case SArgBody:
            if (mArgBodyCnt == 0) {
                if (isKey(split)) {
                    mKey.begin().buf = buf;
                    mKey.begin().pos = pos;
                }
            }
            if (mArgBodyCnt + (end - cursor) > mArgLen) {
                pos += mArgLen - mArgBodyCnt;
                cursor = buf->data() + pos;
                if (*cursor == '\r') {
                    mState = SArgBodyLF;
                    if (isKey(split)) {
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
        case SArgBodyLF:
            if (ch == '\n') {
                if (++mArgCnt == mArgNum) {
                    mState = Finished;
                    goto Done;
                } else {
                    mState = SArgTag;
                    if (isSplit(split)) {
                        goto Done;
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
    if (mState == Finished) {
        return mStatus == Normal ? Complete : mStatus;
    } else {
        return mStatus == Normal ? Partial : ParseError;
    }
}

void RequestParser::parseCmd()
{
    FuncCallTimer();
    if (mArgLen >= Const::MaxCmdLen) {
        mStatus = CmdError;
        mType = Command::None;
        logNotice("unknown request cmd too long:%s...", mCmd);
        return;
    }
    auto c = Command::find(mCmd);
    if (!c) {
        mCommand = &Command::get(Command::None);
        mStatus = CmdError;
        mType = Command::None;
        logNotice("unknown request cmd:%s", mCmd);
        return;
    }
    mType = c->type;
    mCommand = c;
    if (mInline) {
        switch (mType) {
        case Command::Ping:
        case Command::Echo:
        case Command::Auth:
        case Command::Select:
        case Command::Quit:
            break;
        default:
            mStatus = CmdError;
            logNotice("unsupport command %s in inline command protocol", c->name);
            return;
        }
        return;
    }
    if (mArgNum < c->minArgs || mArgNum > c->maxArgs) {
        mStatus = ArgError;
        logNotice("request argument is invalid cmd %s argnum %d",
                mCmd, mArgNum);
        return;
    }
    switch (mType) {
    case Command::Mset:
    case Command::Msetnx:
        if (!(mArgNum & 1)) {
            mStatus = ArgError;
            logNotice("request argument is invalid cmd %s argnum %d",
                    mCmd, mArgNum);
        }
        break;
    default:
        break;
    }
}

