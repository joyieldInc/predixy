/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_LOGGER_H_
#define _PREDIXY_LOGGER_H_

#include <stdarg.h>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "Exception.h"

class LogFileSink;

class LogLevel
{
public:
    enum Type
    {
        Verb,
        Debug,
        Info,
        Notice,
        Warn,
        Error,

        Sentinel
    };
    static const char* Str[Sentinel];
};

class LogUnit
{
public:
    static const int MaxLogLen = 1024;
public:
    LogUnit();
    ~LogUnit();
    void format(LogLevel::Type level, const char* file, int line, const char* fmt, ...);
    void vformat(LogLevel::Type level, const char* file, int line, const char* fmt, va_list ap);
    const char* data() const {return mBuf;}
    int length() const {return mLen;}
private:
    int mLen;
    char mBuf[MaxLogLen];
};

class Logger
{
public:
    DefException(SetLogFileFail);
public:
    Logger(int maxLogUnitNum = 1024);
    ~Logger();
    void start();
    void stop();
    void setLogFile(const char* file, int rotateSecs, long rotateBytes);
    bool allowMissLog() const
    {
        return mAllowMissLog;
    }
    void setAllowMissLog(bool v)
    {
        mAllowMissLog = v;
    }
    int logSample(LogLevel::Type lvl) const
    {
        return mLogSample[lvl];
    }
    void setLogSample(LogLevel::Type lvl, int val)
    {
        mLogSample[lvl] = val;
    }
    LogUnit* log(LogLevel::Type lvl)
    {
        if (mLogSample[lvl] <= 0 || ++LogCnt[lvl] % mLogSample[lvl] != 0) {
            return nullptr;
        }
        return getLogUnit();
    }
    void put(LogUnit* u)
    {
        std::unique_lock<std::mutex> lck(mMtx);
        mLogs.push_back(u);
        mCond.notify_one();
    }
    void log(LogLevel::Type lvl, const char* file, int line, const char* fmt, ...);
    int logFileFd() const;
    static Logger* gInst;
private:
    LogUnit* getLogUnit();
    void run();
private:
    bool mStop;
    bool mAllowMissLog;
    long mMissLogs;
    int mLogSample[LogLevel::Sentinel];
    unsigned mLogUnitCnt;
    std::vector<LogUnit*> mLogs;
    std::vector<LogUnit*> mFree;
    std::mutex mMtx;
    std::condition_variable mCond;
    std::thread* mThread;
    LogFileSink* mFileSink;
    thread_local static long LogCnt[LogLevel::Sentinel];
};

#if 0

#define logVerb(fmt, ...) 
#define logDebug(fmt, ...) 
#define logInfo(fmt, ...) 
#define logNotice(fmt, ...) 
#define logWarn(fmt, ...) 
#define logError(fmt, ...) 

#else

#define logMacroImpl(lvl, fmt, ...) \
    do {                                                                      \
        if (auto _lu_ = Logger::gInst->log(lvl)) {                    \
            _lu_->format(lvl, __FILE__, __LINE__, fmt, ##__VA_ARGS__);\
            Logger::gInst->put(_lu_);                                         \
        }                                                                     \
    } while(0)

#define logVerb(fmt, ...)   logMacroImpl(LogLevel::Verb, fmt, ##__VA_ARGS__)
#define logDebug(fmt, ...)  logMacroImpl(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define logInfo(fmt, ...)   logMacroImpl(LogLevel::Info, fmt, ##__VA_ARGS__)
#define logNotice(fmt, ...) logMacroImpl(LogLevel::Notice, fmt, ##__VA_ARGS__)
#define logWarn(fmt, ...)   logMacroImpl(LogLevel::Warn, fmt, ##__VA_ARGS__)
#define logError(fmt, ...)  logMacroImpl(LogLevel::Error, fmt, ##__VA_ARGS__)

#endif

#endif
