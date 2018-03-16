/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_BACKTRACE_H_
#define _PREDIXY_BACKTRACE_H_

#include "Logger.h"

#if _PREDIXY_BACKTRACE_

#include <execinfo.h>
inline void traceInfo(int sig)
{
#define Size 128
    logError("predixy backtrace(%d)", sig);
    void* buf[Size];
    int num = ::backtrace(buf, Size);
    int fd = -1;
    if (Logger::gInst) {
        fd = Logger::gInst->logFileFd();
        if (fd >= 0) {
            backtrace_symbols_fd(buf, num, fd);
        }
    }
    if (fd != STDOUT_FILENO) {
        backtrace_symbols_fd(buf, num, STDOUT_FILENO);
    }
}

#else

inline void traceInfo(int sig)
{
    logError("predixy backtrace(%d), but current system unspport backtrace", sig);
}

#endif

#endif
