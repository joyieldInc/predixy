/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_STATS_H_
#define _PREDIXY_STATS_H_


struct HandlerStats
{
    long accept                 = 0;
    long clientConnections      = 0;
    long requests               = 0;
    long responses              = 0;
    long recvClientBytes        = 0;
    long sendClientBytes        = 0;
    long sendServerBytes        = 0;
    long recvServerBytes        = 0;

    HandlerStats& operator+=(const HandlerStats& oth)
    {
        accept += oth.accept;
        clientConnections += oth.clientConnections;
        requests += oth.requests;
        responses += oth.responses;
        recvClientBytes += oth.recvClientBytes;
        sendClientBytes += oth.sendClientBytes;
        sendServerBytes += oth.sendServerBytes;
        recvServerBytes += oth.recvServerBytes;

        return *this;
    }
    void reset()
    {
        accept = 0;
        requests = 0;
        responses = 0;
        recvClientBytes = 0;
        sendClientBytes = 0;
        sendServerBytes = 0;
        recvServerBytes = 0;
    }
};

struct ServerStats
{
    int connections     = 0;
    long connect        = 0;
    long close          = 0;
    long requests       = 0;
    long responses      = 0;
    long sendBytes      = 0;
    long recvBytes      = 0;

    ServerStats& operator+=(const ServerStats& oth)
    {
        connections += oth.connections;
        connect += oth.connect;
        close += oth.close;
        requests += oth.requests;
        responses += oth.responses;
        sendBytes += oth.sendBytes;
        recvBytes += oth.recvBytes;
        return *this;
    }
    void reset()
    {
        connect = 0;
        close = 0;
        requests = 0;
        responses = 0;
        sendBytes = 0;
        recvBytes = 0;
    }
};

#endif
