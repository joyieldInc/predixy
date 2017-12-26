/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sstream>
#include "LogFileSink.h"
#include "Logger.h"

static const int DaySecs = 86400;
static const int FileSuffixReserveLen = 20;

LogFileSink::LogFileSink():
    mFilePathLen(0),
    mFileSuffixFmt(nullptr),
    mFile(nullptr),
    mRotateSecs(0),
    mRotateBytes(0),
    mLastReopenTime(0),
    mBytes(0),
    mError(0)
{
}

LogFileSink::~LogFileSink()
{
    if (mFile && mFile != stdout) {
        fclose(mFile);
        mFile = nullptr;
    }
}

bool LogFileSink::parseRotate(const char* rotate, int& secs, long& bytes)
{
    secs = 0;
    bytes = 0;
    if (!rotate || *rotate == '\0') {
        return true;
    }
    std::istringstream iss(rotate);
    std::string s;
    while (iss >> s) {
        int n;
        char unit[4];
        int c = sscanf(s.c_str(), "%d%2s", &n, unit);
        if (c != 2 || n <= 0) {
            return false;
        }
        if (strcmp(unit, "d") == 0) {
            if (n != 1) {
                return false;
            }
            secs = DaySecs;
        } else if (strcmp(unit, "h") == 0) {
            if (n > 24) {
                return false;
            }
            secs = n * 3600;
        } else if (strcmp(unit, "m") == 0) {
            if (n > 1440) {
                return false;
            }
            secs = n * 60;
        } else if (strcmp(unit, "G") == 0) {
            bytes = n * (1 << 30);
        } else if (strcmp(unit, "M") == 0) {
            bytes = n * (1 << 20);
        } else {
            return false;
        }
    }
    return true;
}

time_t LogFileSink::roundTime(time_t t)
{
    if (mRotateSecs > 0) {
        struct tm m;
        localtime_r(&t, &m);
        m.tm_sec = 0;
        m.tm_min = 0;
        m.tm_hour = 0;
        time_t start = mktime(&m);
        return start + (t - start) / mRotateSecs * mRotateSecs;
    }
    return t;
}

bool LogFileSink::reopen(time_t t)
{
    if (mFileSuffixFmt) {
        struct tm m;
        localtime_r(&t, &m);
        char* p = mFilePath + mFilePathLen;
        int len = MaxPathLen - mFilePathLen;
        int num = strftime(p, len, mFileSuffixFmt, &m);
        if (num <= 0) {
            return false;
        }
    }
    if (mFilePathLen > 0) {
        FILE* f = fopen(mFilePath, "a");
        if (!f) {
            return false;
        }
        if (mFile && mFile != stdout) {
            fclose(mFile);
        }
        mFile = f;
        if (mLastReopenTime != t) {
            mBytes = ftell(f);
            mLastReopenTime = t;
        }
        if (mFileSuffixFmt) {
            unlink(mFileName.c_str());
            const char* name = strrchr(mFilePath, '/');
            name = name ? name + 1 : mFilePath;
            if (symlink(name, mFileName.c_str()) == -1) {
                fprintf(stderr, "create symbol link for %s fail", mFileName.c_str());
            }
        }
    } else {
        mFile = stdout;
    }
    return true;
}

bool LogFileSink::setFile(const char* path, int rotateSecs, long rotateBytes)
{
    int len = strlen(path);
    if (rotateSecs > 0 || rotateBytes > 0) {
        if (len > 4 && strcasecmp(path + len - 4, ".log") == 0) {
            len -= 4;
        }
    }
    if (len + FileSuffixReserveLen >= MaxPathLen) {
        return false;
    }
    mFileName = path;
    mFilePathLen = len;
    memcpy(mFilePath, path, len);
    mFilePath[len] = '\0';
    mRotateSecs = rotateSecs;
    mRotateBytes = rotateBytes;
    if (len > 0) {
        if (rotateBytes > 0) {
            mFileSuffixFmt = ".%Y%m%d%H%M%S.log";
        } else if (rotateSecs >= 86400) {
            mFileSuffixFmt = ".%Y%m%d.log";
        } else if (rotateSecs >= 3600) {
            mFileSuffixFmt = ".%Y%m%d%H.log";
        } else if (rotateSecs > 0) {
            mFileSuffixFmt = ".%Y%m%d%H%M.log";
        }
    }
    time_t now = time(nullptr);
    return reopen(mRotateBytes > 0 ? now : roundTime(now));
}

void LogFileSink::checkRotate()
{
    bool rotate = false;
    time_t now = time(nullptr);
    if (mRotateBytes > 0 && mBytes >= mRotateBytes) {
        rotate = reopen(now);
    }
    if (!rotate && mRotateSecs > 0) {
        now = roundTime(now);
        if (now > mLastReopenTime &&
            now - roundTime(mLastReopenTime) >= mRotateSecs) {
            rotate = reopen(now);
        }
    }
}

void LogFileSink::write(const LogUnit* log)
{
    if (mFile) {
        int len = log->length();
        int n = fwrite(log->data(), len, 1, mFile);
        if (n != 1) {
            ++mError;
        } else {
            mBytes += len;
        }
        fflush(mFile);
    }
}
