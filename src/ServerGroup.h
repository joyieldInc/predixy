/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SERVER_GROUP_H_
#define _PREDIXY_SERVER_GROUP_H_

#include <string>
#include <deque>
#include <vector>
#include "Server.h"
#include "String.h"
#include "Predixy.h"

class ServerGroup : public ID<ServerGroup>
{
public:
    ServerGroup(ServerPool* pool, const String& name);
    ~ServerGroup();
    ServerPool* pool() const {return mPool;}
    String name() const {return mName;}
    Server* getMaster() const;
    Server* getServer(Handler* h, Request* req) const;
    void add(Server* s);
    void remove(Server* s);
private:
    Server* getReadServer(Handler* h) const;
    Server* getReadServer(Handler* h, DC* localDC) const;
private:
    ServerPool* mPool;
    SString<Const::MaxServNameLen> mName;
    std::vector<Server*> mServs;
};

#endif
