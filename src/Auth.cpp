/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <algorithm>
#include "Auth.h"
#include "Conf.h"
#include "Request.h"


Auth::Auth(int mode):
    mMode(mode),
    mReadKeyPrefix(nullptr),
    mWriteKeyPrefix(nullptr)
{
}

Auth::Auth(const AuthConf& conf):
    mPassword(conf.password.c_str(), conf.password.size()),
    mMode(conf.mode),
    mReadKeyPrefix(nullptr),
    mWriteKeyPrefix(nullptr)
{
    if (!conf.readKeyPrefix.empty()) {
        mReadKeyPrefix = new KeyPrefixSet(conf.readKeyPrefix.begin(), conf.readKeyPrefix.end());
    }
    if (!conf.writeKeyPrefix.empty()) {
        mWriteKeyPrefix = new KeyPrefixSet(conf.writeKeyPrefix.begin(), conf.writeKeyPrefix.end());
    }
    if ((!mReadKeyPrefix || !mWriteKeyPrefix) && !conf.keyPrefix.empty()) {
        auto kp = new KeyPrefixSet(conf.keyPrefix.begin(), conf.keyPrefix.end());
        if (!mReadKeyPrefix) {
            mReadKeyPrefix = kp;
        }
        if (!mWriteKeyPrefix) {
            mWriteKeyPrefix = kp;
        }
    }
}

Auth::~Auth()
{
    if (mReadKeyPrefix) {
        delete mReadKeyPrefix;
    }
    if (mWriteKeyPrefix && mWriteKeyPrefix != mReadKeyPrefix) {
        delete mWriteKeyPrefix;
    }
}

bool Auth::permission(Request* req, const String& key) const
{
    auto& cmd = Command::get(req->type());
    if (!(mMode & cmd.mode)) {
        return false;
    }
    const KeyPrefixSet* kp = nullptr;
    if (cmd.mode & Command::Read) {
        kp = mReadKeyPrefix;
    } else if (cmd.mode & Command::Write) {
        kp = mWriteKeyPrefix;
    }
    if (kp) {
        auto it = kp->upper_bound(key);
        const String* p = nullptr;
        if (it == kp->end()) {
            p = &(*kp->rbegin());
        } else if (it != kp->begin()) {
            p = &(*--it);
        }
        return p && key.hasPrefix(*p);
    }
    return true;
}

Auth Authority::AuthAllowAll;
Auth Authority::AuthDenyAll(0);

Authority::Authority():
    mDefault(&AuthAllowAll)
{
}

Authority::~Authority()
{
    for (auto& i : mAuthMap) {
        delete i.second;
    }
    mAuthMap.clear();
}

void Authority::add(const AuthConf& ac)
{
    Auth* a = new Auth(ac);
    auto it = mAuthMap.find(a->password());
    if (it != mAuthMap.end()) {
        Auth* p = it->second;
        mAuthMap.erase(it);
        delete p;
    }
    mAuthMap[a->password()] = a;
    if (a->password().empty()) {
        mDefault = a;
    } else if (mDefault == &AuthAllowAll) {
        mDefault = &AuthDenyAll;
    }
}
