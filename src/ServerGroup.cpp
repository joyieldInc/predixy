/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <algorithm>
#include "DC.h"
#include "Proxy.h"
#include "ServerPool.h"
#include "ServerGroup.h"

ServerGroup::ServerGroup(ServerPool* pool, const String& name):
    mPool(pool),
    mName(name)
{
    mServs.reserve(Const::MaxServInGroup);
}

ServerGroup::~ServerGroup()
{
}

Server* ServerGroup::getMaster() const
{
    FuncCallTimer();
    int cnt = mServs.size();
    for (int i = 0; i < cnt; ++i) {
        Server* s = mServs[i];
        if (!s->online()) {
            continue;
        }
        if (s->role() == Server::Master) {
            return s;
        }
    }
    return nullptr;
}

struct ServCond
{
    Server* serv = nullptr;
    int priority = 0;
};

Server* ServerGroup::getServer(Handler* h, Request* req) const
{
    FuncCallTimer();
    Server* serv = nullptr;
    if (req->requireWrite()) {
        int cnt = mServs.size();
        for (int i = cnt-1; i >= 0; --i) {
            Server* s = mServs[i];
            if (!s->online()) {
                continue;
            }
            if (s->role() == Server::Master) {
                serv = s;
                if (!s->fail()){
                    break;
                }
            }
        }
    } else if (auto dataCenter = mPool->proxy()->dataCenter()) {
        serv = getReadServer(h, dataCenter->localDC());
    } else {
        serv = getReadServer(h);
    }
    logDebug("server group %s for req %ld get server %s",
              mName.data(), req->id(), (serv ? serv->addr().data() : "None"));
    return serv;
}

Server* ServerGroup::getReadServer(Handler* h) const
{
    FuncCallTimer();
    Server* servs[Const::MaxServInGroup];
    Server* deadServs[Const::MaxServInGroup];
    int n = 0;
    int dn = 0;
    int prior = 0;
    int dprior = 0;
    int pendRequests = INT_MAX;
    int cnt = mServs.size();
    for (int i = 0; i < cnt; ++i) {
        Server* s = mServs[i];
        if (!s->online()) {
            continue;
        }
        int rp = 0;
        if (s->role() == Server::Master) {
            rp = mPool->masterReadPriority();
        } else if (s->role() == Server::Slave) {
            rp = s->isStatic() ? mPool->staticSlaveReadPriority() : mPool->dynamicSlaveReadPriority();
        }
        if (rp <= 0) {
            continue;
        }
        if (s->fail()) {
            if (rp > dprior) {
                dprior = rp;
                deadServs[0] = s;
                dn = 1;
            } else if (rp == dprior) {
                deadServs[dn++] = s;
            }
        } else {
            if (rp > prior) {
                prior = rp;
                pendRequests = h->getPendRequests(s);
                servs[0] = s;
                n = 1;
            } else if (rp == prior) {
                int preqs = h->getPendRequests(s);
                if (preqs < pendRequests) {
                    pendRequests = preqs;
                    servs[0] = s;
                    n = 1;
                } else if (preqs == pendRequests) {
                    servs[n++] = s;
                }
            }
        }
    }
    Server** ss = nullptr;
    if (n > 0) {
        ss = servs;
    } else if (dn > 0) {
        ss = deadServs;
        n = dn;
    } else {
        return nullptr;
    }
    n = h->rand() % n;
    return ss[n];
}

Server* ServerGroup::getReadServer(Handler* h, DC* localDC) const
{
    FuncCallTimer();
    Server* servs[Const::MaxServInGroup];
    int sprior[Const::MaxServInGroup];
    int pendRequests[Const::MaxServInGroup];
    int num = 0;
    DC* dcs[Const::MaxServInGroup];
    DC* deadDCs[Const::MaxServInGroup];
    int n = 0;
    int dn = 0;
    int prior = 0;
    int dprior = 0;
    int cnt = mServs.size();
    for (int i = 0; i < cnt; ++i) {
        Server* s = mServs[i];
        if (!s->online()) {
            continue;
        }
        int rp = 0;
        if (s->role() == Server::Master) {
            rp = mPool->masterReadPriority();
        } else if (s->role() == Server::Slave) {
            rp = s->isStatic() ? mPool->staticSlaveReadPriority() : mPool->dynamicSlaveReadPriority();
        }
        if (rp <= 0) {
            continue;
        }
        DC* dc = s->dc();
        if (!dc) {
            continue;
        }
        int dcrp = localDC->getReadPriority(dc);
        if (dcrp <= 0) {
            continue;
        }
        servs[num] = s;
        sprior[num] = rp;
        pendRequests[num++] = h->getPendRequests(s);
        if (s->fail()) {
            if (dcrp > dprior) {
                dprior = dcrp;
                deadDCs[0] = dc;
                dn = 1;
            } else if (dcrp == dprior) {
                deadDCs[dn++] = dc;
            }
        } else {
            if (dcrp > prior) {
                prior = dcrp;
                dcs[0] = dc;
                n = 1;
            } else if (dcrp == prior) {
                dcs[n++] = dc;
            }
        }
    }
    DC** sdc = nullptr;
    if (n > 0) {
        sdc = dcs;
    } else if (dn > 0) {
        sdc = deadDCs;
        n = dn;
    } else {
        return nullptr;
    }
    bool found = false;
    DC* dc = nullptr;
    if (n > 1) {
        int m = h->idUnique().unique(sdc, n);
        int weights[Const::MaxServInGroup];
        int w = 0;
        n = 0;
        for (int i = 0; i < m; ++i) {
            int dw = localDC->getReadWeight(sdc[i]);
            if (dw > 0) {
                w += dw;
                sdc[n] = sdc[i];
                weights[n++] = w;
            }
        }
        if (w > 0) {
            w = h->rand() % w;
            int i = std::lower_bound(weights, weights + n, w) - weights;
            dc = sdc[i];
            found = true;
        }
    } else if (localDC->getReadWeight(sdc[0]) > 0) {
        dc = sdc[0];
        found = true;
    }
    if (!found) {
        return nullptr;
    }
    Server* deadServs[Const::MaxServInGroup];
    n = 0;
    dn = 0;
    prior = 0;
    dprior = 0;
    int preqs = INT_MAX;
    for (int i = 0; i < num; ++i) {
        Server* s = servs[i];
        if (servs[i]->dc() != dc) {
            continue;
        }
        if (s->fail()) {
            if (sprior[i] > dprior) {
                dprior = sprior[i];
                deadServs[0] = s;
                dn = 1;
            } else if (sprior[i] == dprior) {
                deadServs[dn++] = s;
            }
        } else {
            if (sprior[i] > prior) {
                prior = sprior[i];
                preqs = pendRequests[i];
                servs[0] = s;
                n = 1;
            } else if (sprior[i] == prior) {
                if (pendRequests[i] < preqs) {
                    preqs = pendRequests[i];
                    servs[0] = s;
                    n = 1;
                } else if (pendRequests[i] == preqs) {
                    servs[n++] = s;
                }
            }
        }
    }
    Server** ss = nullptr;
    if (n > 0) {
        ss = servs;
    } else if (dn > 0) {
        ss = deadServs;
        n = dn;
    } else {
        return nullptr;
    }
    n = h->rand() % n;
    return ss[n];
}

/***************************************************
 * other thread may read ServerGroup when add or remoev,
 * but we don't use lock
 *
 *
 **************************************************/
void ServerGroup::add(Server* s)
{
    if (mServs.size() == mServs.capacity()) {
        return;
    }
    for (unsigned i = 0; i < mServs.size(); ++i) {
        if (mServs[i] == s) {
            return;
        }
    }
    mServs.push_back(s);
}

void ServerGroup::remove(Server* s)
{
    for (unsigned i = 0; i < mServs.size(); ++i) {
        if (mServs[i] == s) {
            mServs[i] = mServs.back();
            mServs.resize(mServs.size() - 1);
            return;
        }
    }
}
