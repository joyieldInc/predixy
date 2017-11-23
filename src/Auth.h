/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_AUTH_H_
#define _PREDIXY_AUTH_H_

#include <map>
#include <set>
#include <vector>
#include "Predixy.h"

class Auth
{
public:
    Auth(int mode = Command::Read|Command::Write|Command::Admin);
    Auth(const AuthConf& conf);
    ~Auth();
    const String& password() const
    {
        return mPassword;
    }
    bool permission(Request* req, const String& key) const;
private:
    String mPassword;
    int mMode;
    typedef std::set<String> KeyPrefixSet;
    KeyPrefixSet* mReadKeyPrefix;
    KeyPrefixSet* mWriteKeyPrefix;
};

class Authority
{
public:
    Authority();
    ~Authority();
    bool hasAuth() const
    {
        return !mAuthMap.empty();
    }
    Auth* get(const String& pd) const
    {
        auto it = mAuthMap.find(pd);
        return it == mAuthMap.end() ? nullptr : it->second;
    }
    Auth* getDefault() const
    {
        return mDefault;
    }
    void add(const AuthConf& ac);
private:
    std::map<String, Auth*> mAuthMap;
    Auth* mDefault;
    static Auth AuthAllowAll;
    static Auth AuthDenyAll;
};

#endif
