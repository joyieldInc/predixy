/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_DC_H_
#define _PREDIXY_DC_H_

#include <vector>
#include <map>
#include "ID.h"
#include "Conf.h"
#include "String.h"

struct DCReadPolicy
{
    int priority = 0;
    int weight = 0;
};

class DC : public ID<DC>
{
public:
    DC(const String& name, int size);
    ~DC();
    const String& name() const
    {
        return mName;
    }
    void set(DC* oth, const ReadPolicyConf& c)
    {
        mReadPolicy[oth->id()].priority = c.priority;
        mReadPolicy[oth->id()].weight = c.weight;
    }
    int getReadPriority(DC* oth) const
    {
        return oth ? mReadPolicy[oth->id()].priority : 1;
    }
    int getReadWeight(DC* oth) const
    {
        return oth ? mReadPolicy[oth->id()].weight : 1;
    }
    const DCReadPolicy& getReadPolicy(DC* oth) const
    {
        return mReadPolicy[oth->id()];
    }
private:
    String mName;
    std::vector<DCReadPolicy> mReadPolicy;
};

class DataCenter
{
public:
    DefException(InitFail);
public:
    DataCenter();
    ~DataCenter();
    void init(const Conf* conf);
    DC* getDC(const String& addr) const;
    DC* localDC() const
    {
        return mLocalDC;
    }
private:
    std::map<String, DC*> mDCs;
    std::map<String, DC*> mAddrDC;
    DC* mLocalDC;
};

#endif
