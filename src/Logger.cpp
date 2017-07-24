/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <chrono>
#include "LogFileSink.h"
#include "Util.h"
#include "Logger.h"
#include <stdio.h>

const char* LogLevel::Str[Sentinel] = {
    "V",
    "D",
    "I",
    "N",
    "W",
    "E"
};

LogUnit::LogUnit():
    mLen(0)
{
}

LogUnit::~LogUnit()
{
    mLen = 0;
}

void LogUnit::format(LogLevel::Type level, const char* file, int line, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vformat(level, file, line, fmt, ap);
    va_end(ap);
}

void LogUnit::vformat(LogLevel::Type level, const char* file, int line, const char* fmt, va_list ap)
{
    long us = Util::nowUSec();
    time_t t = us / 1000000;
    struct tm m;
    localtime_r(&t, &m);
    char* p = mBuf;
    size_t len = MaxLogLen;
    int n = strftime(p, len, "%Y-%m-%d %H:%M:%S", &m);
    p += n;
    mLen = n;
    len -= n;
    us %= 1000000;
    n = snprintf(p, len, ".%06ld %s %s:%d ", us, LogLevel::Str[level], file, line);
    p += n;
    mLen += n;
    len -= n;
    n = vsnprintf(p, len, fmt, ap);
    mLen += n;
    if (mLen >= MaxLogLen) {
        mLen = MaxLogLen - 1;
    }
    mBuf[mLen++] = '\n';
}

thread_local long Logger::LogCnt[LogLevel::Sentinel]{0, 0, 0, 0, 0, 0};
Logger* Logger::gInst = NULL;

Logger::Logger(int maxLogUnitNum):
    mStop(false),
    mAllowMissLog(true),
    mMissLogs(0),
    mLogSample{0, 0, 100, 1, 1, 1},
    mLogUnitCnt(0),
    mLogs(maxLogUnitNum),
    mFree(maxLogUnitNum),
    mFileSink(nullptr)
{
    mLogs.resize(0);
    mFree.resize(0);
}

Logger::~Logger()
{
    if (mFileSink) {
        delete mFileSink;
    }
    if (mThread) {
        delete mThread;
    }
}

void Logger::setLogFile(const char* file, int rotateSecs, long rotateBytes)
{
    if (!mFileSink) {
        mFileSink = new LogFileSink();
    }
    if (!mFileSink->setFile(file, rotateSecs, rotateBytes)) {
        Throw(SetLogFileFail, "set log file %s fail %s", file, StrError());
    }
}

void Logger::start()
{
    mThread = new std::thread([=](){this->run();});
    mThread->detach();
}

void Logger::stop()
{
    mStop = true;
    std::unique_lock<std::mutex> lck(mMtx);
    mCond.notify_one();
}

void Logger::run()
{
    std::vector<LogUnit*> logs(mFree.capacity());
    logs.resize(0);
    while (!mStop) {
        long missLogs = 0;
        do {
            std::unique_lock<std::mutex> lck(mMtx);
            while (mLogs.empty() && !mStop) {
                mCond.wait(lck);
            }
            logs.swap(mLogs);
            missLogs = mMissLogs;
            mMissLogs = 0;
        } while (false);
        if (mFileSink) {
            mFileSink->checkRotate();
            for (auto log : logs) {
                mFileSink->write(log);
                std::unique_lock<std::mutex> lck(mMtx);
                mFree.push_back(log);
                mCond.notify_one();
            }
        } else {
            std::unique_lock<std::mutex> lck(mMtx);
            for (auto log : logs) {
                mFree.push_back(log);
            }
            mCond.notify_one();
        }
        logs.resize(0);
        if (missLogs > 0 && mFileSink) {
            LogUnit log;
            log.format(LogLevel::Notice, __FILE__, __LINE__, "MissLog count %ld", missLogs);
            mFileSink->write(&log);
        }
    }
}

void Logger::log(LogLevel::Type lvl, const char* file, int line, const char* fmt, ...)
{
    LogUnit* log = getLogUnit();
    if (!log) {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    log->vformat(lvl, file, line, fmt, ap);
    va_end(ap);
    put(log);
}

LogUnit* Logger::getLogUnit()
{
    LogUnit* log = nullptr;
    if (mAllowMissLog) {
        std::unique_lock<std::mutex> lck(mMtx, std::try_to_lock_t());
        if (!lck) {
            ++mMissLogs;
            return nullptr;
        }
        if (mFree.size() > 0) {
            log = mFree.back();
            mFree.resize(mFree.size() - 1);
        } else if (mLogUnitCnt < mFree.capacity()) {
            ++mLogUnitCnt;
        } else {
            ++mMissLogs;
            return nullptr;
        }
    } else {
        std::unique_lock<std::mutex> lck(mMtx);
        if (!mFree.empty()) {
            log = mFree.back();
            mFree.resize(mFree.size() - 1);
        } else if (mLogUnitCnt < mFree.capacity()) {
            ++mLogUnitCnt;
        } else {
            while (mFree.empty() && !mStop) {
                mCond.wait(lck);
            }
            if (!mFree.empty()) {
                log = mFree.back();
                mFree.resize(mFree.size() - 1);
            } else {
                return nullptr;
            }
        }
    }
    return log ? log : new LogUnit();
}

int Logger::logFileFd() const
{
    return mFileSink ? mFileSink->fd() : -1;
}
