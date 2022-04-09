/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_SERVER_H_
#define _PREDIXY_SERVER_H_

#include <string>
#include "Predixy.h"
#include "DC.h"
#include "ConnectConnection.h"

class Server : public ID<Server>
{
public:
    enum Role
    {
        Unknown,
        Master,
        Slave,
        Sentinel
    };
    static const char* RoleStr[];
public:
    Server(ServerPool* pool, const String& addr, bool isStatic);
    ~Server();
    bool activate();
    void incrFail();
    ServerPool* pool() const
    {
        return mPool;
    }
    ServerGroup* group() const
    {
        return mGroup;
    }
    void setGroup(ServerGroup* g)
    {
        mGroup = g;
    }
    void setRole(Role role)
    {
        mRole = role;
    }
    Role role() const
    {
        return mRole;
    }
    const char* roleStr() const
    {
        return RoleStr[mRole];
    }
    bool isMaster() const
    {
        return mRole == Master;
    }
    bool isSlave() const
    {
        return mRole == Slave;
    }
    bool isStatic() const
    {
        return mStatic;
    }
    const String& name() const
    {
        return mName;
    }
    void setName(const String& str)
    {
        mName = str;
    }
    const String& masterName() const
    {
        return mMasterName;
    }
    void setMasterName(const String& str)
    {
        mMasterName = str;
    }
    const String& addr() const
    {
        return mAddr;
    }
    const String& password() const
    {
        return mPassword;
    }
    void setPassword(const String& pw)
    {
        mPassword = pw;
    }
    DC* dc() const
    {
        return mDC;
    }
    String dcName() const
    {
        return mDC ? mDC->name() : String("", 0);
    }
    bool fail() const
    {
        return mFail;
    }
    void setFail(bool v)
    {
        logError("ibk: SET setFail:%d", v);
        mFail = v;
    }
    bool online() const
    {
        return mOnline;
    }
    void setOnline(bool v)
    {
        logError("[ibk: SET_ONLINE SetOnline:%d", v);
        mOnline = v;
    }
    bool updating() const
    {
        return mUpdating;
    }
    void setUpdating(bool v)
    {
        mUpdating = v;
    }
private:
    ServerPool* mPool;
    ServerGroup* mGroup;
    Role mRole;
    DC* mDC;
    SString<Const::MaxAddrLen> mAddr;
    SString<Const::MaxServNameLen> mName;
    SString<Const::MaxServNameLen> mMasterName;
    String mPassword;
    AtomicLong mNextActivateTime; //us
    AtomicLong mFailureCnt;
    bool mStatic;
    bool mFail;
    bool mOnline;
    bool mUpdating;
};

#endif
