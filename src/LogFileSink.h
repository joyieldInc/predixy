/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_LOG_FILE_SINK_H_
#define _PREDIXY_LOG_FILE_SINK_H_

#include <stdio.h>
#include <atomic>
#include <string>

class LogUnit;

class LogFileSink
{
public:
    LogFileSink();
    ~LogFileSink();
    bool setFile(const char* path, int rotateSecs = 0, long rotateBytes = 0);
    void checkRotate();
    void write(const LogUnit* log);
    int fd() const
    {
        return mFile ? fileno(mFile) : -1;
    }

    static bool parseRotate(const char* rotate, int& secs, long& bytes);
    static const int MaxPathLen = 1024;
private:
    time_t roundTime(time_t t);
    bool reopen(time_t t);
private:
    std::string mFileName;
    char mFilePath[MaxPathLen];
    int mFilePathLen;
    const char* mFileSuffixFmt;
    FILE* mFile;
    int mRotateSecs;
    long mRotateBytes;
    time_t mLastReopenTime;
    long mBytes;
    long mError;
};

#endif
