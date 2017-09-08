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
    enum State
    {
        Idle,        // * or inline command
        InlineBegin,
        InlineCmd,
        InlineArgBegin,
        InlineArg,
        InlineArgEnd,
        ArgNum,      // 2
        ArgNumLF,    // \r\n
        CmdTag,
        CmdLen,
        CmdLenLF,
        CmdBody,
        CmdBodyTooLong,
        CmdBodyLF,
        KeyTag,
        KeyLen,
        KeyLenLF,
        KeyBody,
        KeyBodyLF,
        ArgTag,
        ArgLen,
        ArgLenLF,
        ArgBody,
        ArgBodyLF,
        SArgTag,
        SArgLen,
        SArgLenLF,
        SArgBody,
        SArgBodyLF,
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
    template<int Size>
    static bool decodeInlineArg(SString<Size>& dst, const String& src);
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
    const char* cmd() const
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
    bool isSplit(bool split) const;
private:
    Command::Type mType;
    const Command* mCommand;
    Segment mReq;
    Segment mKey;
    char mCmd[Const::MaxCmdLen];
    Status mStatus;
    State mState;
    bool mInline;
    bool mEscape;
    char mQuote;
    int mArgNum;
    int mArgCnt;
    int mArgLen;
    int mArgBodyCnt;
    int mByteCnt;
};

template<int Size>
bool RequestParser::decodeInlineArg(SString<Size>& dst, const String& src)
{
    bool ret = true;
    bool escape = false;
    char quote = '\0';
    const char* p = src.data();
    for (int i = 0; i < src.length(); ++i, ++p) {
        char c = *p;
        if (escape) {
            if (quote == '"') {
                switch (c) {
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                case 'b': c = '\b'; break;
                case 'a': c = '\a'; break;
                default: break;
                }
            } else {
                if (!dst.append('\\')) {
                    ret = false;
                }
            }
            escape = false;
        } else if (quote) {
            if (c == '\\') {
                escape = true;
                continue;
            } else if (c == quote) {
                quote = '\0';
                continue;
            }
        } else if (c == '"' || c == '\'') {
            quote = c;
            continue;
        }
        if (!dst.append(c)) {
            ret = false;
        }
    }
    return ret;
}

#endif
