/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include "DC.h"

DC::DC(const String& name, int size):
    mName(name)
{
    mReadPolicy.resize(size);
}

DC::~DC()
{
}

DataCenter::DataCenter():
    mLocalDC(nullptr)
{
}

DataCenter::~DataCenter()
{
}

void DataCenter::init(const Conf* conf)
{
    for (auto& c : conf->dcConfs()) {
        if (mDCs.find(c.name) != mDCs.end()) {
            Throw(InitFail, "DC \"%s\" duplicate define", c.name.c_str());
        }
        DC* dc = new DC(c.name, conf->dcConfs().size());
        mDCs[c.name] = dc;
        if (c.name == conf->localDC()) {
            mLocalDC = dc;
        }
        for (auto& addr : c.addrPrefix) {
            if (mAddrDC.find(addr) != mAddrDC.end()) {
                Throw(InitFail, "DC \"%s\" AddrPrefix \"%s\" collision with DC \"%s\"",
                        c.name.c_str(), addr.c_str(), mAddrDC[addr]->name().data());
            }
            mAddrDC[addr] = dc;
        }
    }
    if (!mLocalDC) {
        Throw(InitFail, "DataCenter can't find localDC \"%s\"",
                conf->localDC().c_str());
    }
    for (auto& c : conf->dcConfs()) {
        DC* dc = mDCs[c.name];
        for (auto& rp : c.readPolicy) {
            auto it = mDCs.find(rp.name);
            if (it == mDCs.end()) {
                Throw(InitFail, "DC \"%s\" ReadPolicy \"%s\" no exists in DataCenter",
                       dc->name().data(), rp.name.c_str());
            }
            dc->set(it->second, rp);
        }
    }
}

DC* DataCenter::getDC(const String& addr) const
{
    String p = addr;
    while (true) {
        auto it = mAddrDC.upper_bound(p);
        if (it == mAddrDC.begin()) {
            return nullptr;
        }
        --it;
        if (p.hasPrefix(it->first)) {
            return it->second;
        }
        p = p.commonPrefix(it->first);
    }
    return nullptr;
}
