/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_EXCEPTION_H_
#define _PREDIXY_EXCEPTION_H_

#include <exception>
#include <stdarg.h>
#include <stdio.h>

class ExceptionBase : public std::exception
{
public:
    static const int MaxMsgLen = 1024;
public:
    ExceptionBase()
    {
        mMsg[0] = '\0';
    }
    ExceptionBase(const char* file, int line, const char* fmt, ...)
    {
        int n = snprintf(mMsg, sizeof(mMsg), "%s:%d ", file, line);
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(mMsg + n, sizeof(mMsg) - n, fmt, ap);
        va_end(ap);
    }
    ExceptionBase(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(mMsg, sizeof(mMsg), fmt, ap);
        va_end(ap);
    }
    ~ExceptionBase()
    {
    }
    const char* what() const noexcept
    {
        return mMsg;
    }
protected:
    void init(const char* fmt, va_list ap)
    {
        vsnprintf(mMsg, sizeof(mMsg), fmt, ap);
    }
private:
    char mMsg[MaxMsgLen];
};

#define DefException(T) class T : public ExceptionBase  \
{                                                       \
public:                                                 \
    template<class... A>                                \
    T(A&&... args):ExceptionBase(args...) {}            \
}

#define Throw(T, ...) throw T(__FILE__, __LINE__, ##__VA_ARGS__)

#endif
