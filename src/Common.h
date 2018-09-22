/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_COMMON_H_
#define _PREDIXY_COMMON_H_

#include <limits.h>

#define _PREDIXY_NAME_      "predixy"
#define _PREDIXY_VERSION_   "1.0.5"

namespace Const
{
    static const long MaxMemoryLower = 10 << 20; //10MB
    static const int MaxBufListNodeNum = 64;
    static const int MinBufSize = 1;
    static const int MinBufSpaceLeft = 1;//64;
    static const int RedisClusterSlots = 16384;
    static const int RedisClusterSlotsMask = 16383;
    static const int MaxServNameLen = 64;       //nodeid in redis cluster; master name in sentinel
    static const int MaxServNum = 2048;
    static const int ServGroupBits = 10;
    static const int MaxServGroupNum = 1<<ServGroupBits;
    static const int ServGroupMask = ((1<<ServGroupBits) - 1);
    static const int MaxServInGroup = 64;
    static const int MaxAddrLen = 128;
    static const int MaxDcLen = 32;
    static const int MaxIOVecLen = IOV_MAX;
    static const int MaxCmdLen = 32;
    static const int MaxKeyLen = 512;
    static const int BufferAllocCacheSize = 64;
    static const int RequestAllocCacheSize = 128;
    static const int ResponseAllocCacheSize = 128;
    static const int AcceptConnectionAllocCacheSize = 32;
    static const int ConnectConnectionAllocCacheSize = 4;
};


#endif
