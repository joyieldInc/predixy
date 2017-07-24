/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "String.h"
#include "ResponseParser.h"

ResponseParser::ResponseParser()
{
    reset();
}

ResponseParser::~ResponseParser()
{
}

void ResponseParser::reset()
{
    mType = Reply::None;
    mRes.clear();
    mState = Idle;
    mInteger = 0;
    mStringLen = 0;
    mStringCnt = 0;
    mDepth = 0;
    mSign = false;
}

ResponseParser::Status ResponseParser::parse(Buffer* buf, int& pos)
{
    FuncCallTimer();
    char* cursor = buf->data() + pos;
    char* end = buf->tail();
    int error = 0;
    while (cursor < end && !error) {
        char ch = *cursor;
        switch (mState) {
        case Idle:
            mRes.begin().buf = buf;
            mRes.begin().pos = cursor - buf->data();
            mRes.rewind();
            switch (ch) {
            case '+':
                mType = Reply::Status;
                mState = StatusHead;
                break;
            case '-':
                mType = Reply::Error;
                mState = ErrorHead;
                break;
            case ':':
                mType = Reply::Integer;
                mState = IntegerHead;
                break;
            case '$':
                mType = Reply::String;
                mState = StringHead;
                break;
            case '*':
                mType = Reply::Array;
                mState = ArrayHead;
                break;
            default:
                error = __LINE__;
                break;
            }
            break;
        case StatusHead:
        case ErrorHead:
            if (ch == '\r') {
                mState = HeadLF;
            }
            break;
        case IntegerHead:
        case StringHead:
        case ArrayHead:
            if (ch >= '0' && ch <= '9') {
                mInteger = mInteger * 10 + ch - '0';
            } else if (ch == '\r') {
                if (mSign) {
                    mInteger = -mInteger;
                }
                mState = HeadLF;
            } else if (ch == '-' && !mSign) {
                mSign = true;
            } else {
                error = __LINE__;
            }
            break;
        case HeadLF:
            if (ch != '\n') {
                error = __LINE__;
                break;
            }
            switch (mType) {
            case Reply::Status:
            case Reply::Error:
            case Reply::Integer:
                goto Done;
            case Reply::String:
                if (mInteger < 0) {
                    goto Done;
                }
                mStringLen = mInteger;
                mStringCnt = 0;
                mState = StringBody;
                break;
            case Reply::Array:
                if (mInteger <= 0) {
                    goto Done;
                }
                mDepth = 0;
                mArrayNum[mDepth] = mInteger;
                mElementCnt[mDepth++] = 0;
                mState = ArrayBody;
                break;
            default:
                error = __LINE__;
                break;
            }
            break;
        case StringBody:
            if (mStringCnt == mStringLen) {
                ch == '\r' ? mState = StringBodyLF : error = __LINE__;
            } else {
                if (mStringCnt + (end - cursor) >= mStringLen) {
                    cursor += mStringLen - mStringCnt - 1;
                    mStringCnt = mStringLen;
                } else {
                    mStringCnt += end - cursor;
                    cursor = end - 1;
                }
            }
            break;
        case StringBodyLF:
            if (ch == '\n') {
                goto Done;
            }
            error = __LINE__;
            break;
        case ArrayBody:
            switch (ch) {
            case '+':
            case '-':
            case ':':
                mState = LineBody;
                break;
            case '$':
                mSign = false;
                mInteger = 0;
                mState = SubStringLen;
                break;
            case '*':
                mInteger = 0;
                mState = SubArrayLen;
                break;
            default:
                error = __LINE__;
                break;
            }
            break;
        case LineBody:
            if (ch == '\r') {
                mState = ElementLF;
            }
            break;
        case SubStringLen:
            if (ch >= '0' && ch <= '9') {
                mInteger = mInteger * 10 + ch - '0';
            } else if (ch == '\r') {
                mState = mSign ? ElementLF : SubStringLenLF;
            } else if (ch == '-' && !mSign) {
                mSign = true;
            } else {
                error = __LINE__;
            }
            break;
        case SubArrayLen:
            if (ch >= '0' && ch <= '9') {
                mInteger = mInteger * 10 + ch - '0';
            } else if (ch == '\r') {
                mState = mInteger > 0 ? SubArrayLenLF : ElementLF;
            } else {
                error = __LINE__;
            }
            break;
        case SubStringLenLF:
            mStringLen = mInteger;
            mStringCnt = 0;
            ch == '\n' ? mState = SubStringBody : error = __LINE__;
            break;
        case SubArrayLenLF:
            if (ch == '\n') {
                if (mDepth < MaxArrayDepth) {
                    mArrayNum[mDepth] = mInteger;
                    mElementCnt[mDepth++] = 0;
                    mState = ArrayBody;
                } else {
                    logError("response parse error:array too depth");
                    error = __LINE__;
                }
            } else {
                error = __LINE__;
            }
            break;
        case SubStringBody:
            if (mStringCnt + (end - cursor) > mStringLen) {
                cursor += mStringLen - mStringCnt;
                *cursor == '\r' ? mState = ElementLF : error = __LINE__;
            } else {
                mStringCnt += end - cursor;
                cursor = end - 1;
            }
            break;
        case ElementLF:
            if (ch != '\n') {
                error = __LINE__;
                break;
            }
            mState = ArrayBody;
            while (true) {
                if (++mElementCnt[mDepth - 1] == mArrayNum[mDepth - 1]) {
                    if (--mDepth == 0) {
                        goto Done;
                    }
                } else {
                    break;
                }
            }
            break;
        default:
            error = __LINE__;
            break;
        }
        ++cursor;
    }
    if (error) {
        SString<64> bufHex;
        bufHex.printHex(buf->data() + pos, buf->length() - pos);
        SString<16> errHex;
        errHex.printHex(cursor - 1, end - cursor + 1);
        logError("response parse error %d state %d buf:%s errpos %d err:%s",
                error, mState, bufHex.data(),
                cursor - 1 - buf->data() - pos, errHex.data());
        return ParseError;
    }
    pos = cursor + 1 - buf->data();
    mRes.end().buf = buf;
    mRes.end().pos = pos;
    return (mState > HeadLF) ? Partial : Normal;
Done:
    mState = Finished;
    pos = cursor + 1 - buf->data();
    mRes.end().buf = buf;
    mRes.end().pos = pos;
    return Complete;
}

