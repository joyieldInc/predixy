/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_IOVEC_H_
#define _PREDIXY_IOVEC_H_


#include <sys/uio.h>

class Request;
class Response;
class Segment;

struct IOVec
{
    char* dat;
    int len;
    int pos;
    Buffer* buf;
    Segment* seg;
    Request* req;
};




#endif
