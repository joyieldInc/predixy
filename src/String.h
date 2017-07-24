/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_STRING_h_
#define _PREDIXY_STRING_h_

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

class String
{
public:
    String():
        mLen(0),
        mDat(nullptr)
    {
    }
    String(const char* str):
        mLen(strlen(str)),
        mDat(str)
    {
    }
    String(const char* dat, int len):
        mLen(len),
        mDat(dat)
    {
    }
    String(const std::string& s):
        mLen(s.size()),
        mDat(s.c_str())
    {
    }
    String(const String& str):
        mLen(str.mLen),
        mDat(str.mDat)
    {
    }
    String(String&& str):
        mLen(str.mLen),
        mDat(str.mDat)
    {
    }
    String& operator=(const String& str)
    {
        if (this != &str) {
            mDat = str.mDat;
            mLen = str.mLen;
        }
        return *this;
    }
    String& operator=(const std::string& str)
    {
        mDat = str.c_str();
        mLen = str.size();
        return *this;
    }
    String& operator=(const char* str)
    {
        mDat = str;
        mLen = str ? strlen(str) : 0;
        return *this;
    }
    operator const char*() const
    {
        return mDat;
    }
    bool operator<(const String& s) const
    {
        int r = strncmp(mDat, s.mDat, mLen < s.mLen ? mLen : s.mLen);
        return r < 0 || (r == 0 && mLen < s.mLen);
    }
    bool operator==(const char* s) const
    {
        return s ? (mLen == (int)strlen(s) && strncmp(mDat, s, mLen) == 0) : mLen == 0;
    }
    bool operator==(const String& s) const
    {
        return mLen == s.mLen && (mLen > 0 ? (strncmp(mDat, s.mDat, mLen) == 0) : true);
    }
    bool operator!=(const String& s) const
    {
        return !operator==(s);
    }
    bool equal(const String& s, bool ignoreCase= false) const
    {
        if (ignoreCase) {
            return mLen == s.mLen && (mLen > 0 ? (strncasecmp(mDat, s.mDat, mLen) == 0) : true);
        }
        return mLen == s.mLen && (mLen > 0 ? (strncmp(mDat, s.mDat, mLen) == 0) : true);
    }
    bool empty() const
    {
        return mLen == 0;
    }
    const char* data() const
    {
        return mDat;
    }
    int length() const
    {
        return mLen;
    }
    bool hasPrefix(const String& s) const
    {
        return s.mLen <= mLen ? (memcmp(mDat, s.mDat, s.mLen) == 0): false;
    }
    String commonPrefix(const String& s) const
    {
        int cnt = 0;
        int len = mLen < s.mLen ? mLen : s.mLen;
        while (cnt < len && mDat[cnt] == s.mDat[cnt]) {
            ++cnt;
        }
        return String(mDat, cnt);
    }
    bool toInt(int& v) const
    {
        v = 0;
        int i = 0;
        int sign = 1;
        if (i > 0) {
            if (mDat[0] == '+') {
                ++i;
            } else if (mDat[0] == '+') {
                sign = -1;
                ++i;
            } else {
                return false;
            }
        }
        for ( ; i < mLen; ++i) {
            if (mDat[i] >= '0' && mDat[i] <= '9') {
                v = v * 10 + (mDat[i] - '0');
            } else {
                return false;
            }
        }
        v *= sign;
        return true;
    }
protected:
    int mLen;
    const char* mDat;
};

class StringCaseCmp
{
public:
    bool operator()(const String& s1, const String& s2) const
    {
        int r = strncasecmp(s1.data(), s2.data(), s1.length() < s2.length() ? s1.length() : s2.length());
        return r < 0 || (r == 0 && s1.length() < s2.length());
    }
};

template <int Size>
class SString : public String
{
public:
    SString():
        String(mBuf, 0)
    {
        mBuf[0] = '\0';
    }
    SString(const char* str):
        String(mBuf, 0)
    {
        set(str);
    }
    SString(const char* dat, int len):
        String(mBuf, 0)
    {
        set(dat, len);
    }
    SString(const std::string& str)
    {
        set(str.c_str(), str.length());
    }
    SString(const SString& str)
    {
        set(str.mDat, str.mLen);
    }
    template<class T>
    SString(const T& str)
    {
        set(str.data(), str.length());
    }
    SString& operator=(const SString& str)
    {
        if (this != &str) {
            set(str.data(), str.length());
        }
        return *this;
    }
    template<class T>
    SString& operator=(const T& str)
    {
        set(str.data(), str.length());
        return *this;
    }
    void clear()
    {
        mLen = 0;
        mBuf[0] = '\0';
    }
    int size() const
    {
        return Size;
    }
    bool set(const char* str)
    {
        return set(str, str ? strlen(str) : 0);
    }
    bool set(const char* dat, int len)
    {
        mDat = mBuf;
        if ((mLen = len < Size ? len : Size) > 0 ) {
            memcpy(mBuf, dat, mLen);
        }
        mBuf[mLen < 0 ? 0 : mLen] = '\0';
        return len <= Size;
    }
    bool printf(const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        bool ret = vprintf(fmt, ap);
        va_end(ap);
        return ret;
    }
    bool vprintf(const char* fmt, va_list ap)
    {
        mDat = mBuf;
        int len = vsnprintf(mBuf, Size, fmt, ap);
        mLen = len < Size ? len : Size;
        return mLen == len;
    }
    bool strftime(const char* fmt, time_t t)
    {
        struct tm m;
        localtime_r(&t, &m);
        int ret = ::strftime(mBuf, Size, fmt, &m);
        return ret > 0 && ret < Size;
    }
    bool append(char c)
    {
        if (mLen < Size) {
            if (mLen < 0) {
                mLen = 0;
            }
            mBuf[mLen++] = c;
            mBuf[mLen] = '\0';
            return true;
        }
        return false;
    }
    bool append(const char* str)
    {
        return append(str, strlen(str));
    }
    bool append(const char* dat, int len)
    {
        if (dat && len >= 0) {
            if (mLen < 0) {
                mLen = 0;
            }
            int room = Size - mLen;
            memcpy(mBuf + mLen, dat, len < room ? len : room);
            mLen += len < room ? len : room;
            mBuf[mLen] = '\0';
            return len <= room;
        }
        return true;
    }
    bool printHex(const char* dat, int len)
    {
        mDat = mBuf;
        int i;
        int n = 0;
        for (i = 0; i < len && n < Size; ++i) {
            switch (dat[i]) {
            case '\r':
                if (n + 1 < Size) {
                    mBuf[n++] = '\\';
                    mBuf[n++] = 'r';
                } else {
                    i = len;
                }
                break;
            case '\n':
                if (n + 1 < Size) {
                    mBuf[n++] = '\\';
                    mBuf[n++] = 'n';
                } else {
                    i = len;
                }
                break;
            default:
                if (isgraph(dat[i])) {
                    mBuf[n++] = dat[i];
                } else if (n + 4 < Size) {
                    snprintf(mBuf + n, Size - n, "\\x%02X", (int)dat[i]);
                    n += 4;
                } else {
                    i = len;
                }
                break;
            }
        }
        mBuf[n] = '\0';
        mLen = n;
        return mLen < Size;
    }
protected:
    char mBuf[Size + 1];
};


#endif
