/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <time.h>
#include "ClusterServerPool.h"
#include "ClusterNodesParser.h"
#include "ServerGroup.h"
#include "Handler.h"

const char* ClusterServerPool::HashTag = "{}";

ClusterServerPool::ClusterServerPool(Proxy* p):
    ServerPoolTmpl(p, Cluster),
    mHash(Hash::Crc16)
{
    mServPool.reserve(Const::MaxServNum);
    mGroupPool.reserve(Const::MaxServGroupNum);
    for (int i = 0; i < Const::RedisClusterSlots; ++i) {
        mSlots[i] = nullptr;
    }
}

ClusterServerPool::~ClusterServerPool()
{
    for (auto i : mServPool) {
        delete i;
    }
    for (auto i : mGroupPool) {
        delete i;
    }
}

Server* ClusterServerPool::getServer(Handler* h, Request* req, const String& key) const
{
    FuncCallTimer();
    switch (req->type()) {
    case Command::ClusterNodes:
    case Command::Randomkey:
        return randServer(h, mServPool);
    default:
        break;
    }
    int i = mHash.hash(key.data(), key.length(), HashTag);
    i &= Const::RedisClusterSlotsMask;
    ServerGroup* g = mSlots[i];
    if (!g) {
        return randServer(h, mServPool);
    }
    return g->getServer(h, req);
}

Server* ClusterServerPool::redirect(const String& addr, Server* old) const
{
    Server* serv = nullptr;
    int size = mServPool.size();
    for (int i = 0; i < size; ++i) {
        Server* s = mServPool[i];
        if (s->addr() == addr) {
            serv = s;
            break;
        }
    }
    return serv;
}

void ClusterServerPool::init(const ClusterServerPoolConf& conf)
{
    ServerPool::init(conf);
    mServPool.resize(conf.servers.size());
    int i = 0;
    for (auto& sc : conf.servers) {
        Server* s = new Server(this, sc.addr.c_str(), true);
        s->setPassword(sc.password.empty() ? conf.password : sc.password);
        mServPool[i++] = s;
        mServs[s->addr()] = s;
    }
}

void ClusterServerPool::refreshRequest(Handler* h)
{
    logDebug("h %d update redis cluster pool", h->id());
    RequestPtr req = RequestAlloc::create(Request::ClusterNodes);
    h->handleRequest(req);
}

void ClusterServerPool::handleResponse(Handler* h, ConnectConnection* s, Request* req, Response* res)
{
    ClusterNodesParser p;
    p.set(res->body());
    for (auto serv : mServPool) {
        serv->setUpdating(true);
    }
    while (true) {
        ClusterNodesParser::Status st = p.parse();
        if (st == ClusterNodesParser::Node) { // it is a node
            logDebug("redis cluster update parse node %s %s %s %s",
                     p.nodeId().data(),
                     p.addr().data(),
                     p.flags().data(),
                     p.master().data());
            if (p.addr().empty()) {
                logWarn("redis cluster nodes get node invalid %s %s %s %s",
                        p.nodeId().data(),
                        p.addr().data(),
                        p.flags().data(),
                        p.master().data());
                continue;
            }
            String addr(p.addr());
            auto it = mServs.find(addr);
            Server* serv = it == mServs.end() ? nullptr : it->second;
            // the raw address is not in our server list, check further for these:
            // - myself
            // - if using `@` separator
            if (!serv) {
                if (strstr(p.flags().data(), "myself")) { // the node is where the command get executed
                    serv = s->server();
                } else if (const char* t = strchr(p.addr().data(), '@')) {
                    addr = String(p.addr().data(), t - p.addr().data());
                    it = mServs.find(addr);
                    serv = it == mServs.end() ? nullptr : it->second;
                }
            }
            // still not found in our server list, check further:
            // - unknown role || fair || handshake -> ignore
            // - num of server already exceed the pool capacity -> ignore it
            // if not above, create new server object and add to the mServPool & mServs
            if (!serv) {
                const char* flags = p.flags().data();
                if (p.role() == Server::Unknown ||
                    strstr(flags, "fail") || strstr(flags, "handshake")) {
                    logNotice("redis cluster nodes get node abnormal %s %s %s %s",
                            p.nodeId().data(),
                            p.addr().data(),
                            p.flags().data(),
                            p.master().data());
                    continue;
                }
                if (mServPool.size() == mServPool.capacity()) {
                    logWarn("cluster server pool had too many servers %d, will ignore new server %s",
                            (int)mServPool.size(), p.addr().data());
                    continue;
                }
                serv = new Server(this, addr, false);
                serv->setPassword(password());
                mServPool.push_back(serv);
                mServs[serv->addr()] = serv;
                logNotice("redis cluster create new server %s %s %s %s %s",
                            p.nodeId().data(),
                            p.addr().data(),
                            p.flags().data(),
                            p.master().data(),
                            serv->dcName().data());
            } else {
                serv->setUpdating(false);
            }
            serv->setRole(p.role());
            serv->setName(p.nodeId());
            serv->setMasterName(p.master());
            // if it is master
            // - add group if needed
            // - add the slot
            if (p.role() == Server::Master) {
                ServerGroup* g = getGroup(p.nodeId());
                if (!g) {
                    if (mGroupPool.size() == mGroupPool.capacity()) {
                        logNotice("redis cluster too many group %s %s %s %s",
                                p.nodeId().data(),
                                p.addr().data(),
                                p.flags().data(),
                                p.master().data());
                        continue;
                    }
                    g = new ServerGroup(this, p.nodeId());
                    mGroupPool.push_back(g);
                    mGroups[g->name()] = g;
                    logNotice("redis cluster create new group %s %s %s %s",
                            p.nodeId().data(),
                            p.addr().data(),
                            p.flags().data(),
                            p.master().data());
                }
                g->add(serv);
                int begin, end;
                if (p.getSlot(begin, end)) {
                    while (begin < end) {
                        mSlots[begin++] = g;
                    }
                }
            }
        } else if (st == ClusterNodesParser::Finished) {
            logDebug("redis cluster nodes update finish");
            break;
        } else {
            logError("redis cluster nodes parse error");
            return;
        }
    }
    // assign to proper group
    // there is no health check here
    for (auto serv : mServPool) {
        if (serv->updating()) {
            serv->setUpdating(false);
            continue;
        }
        if (serv->role() == Server::Master) {
            ServerGroup* g = getGroup(serv->name());
            if (serv->group() && serv->group() != g) {
                serv->group()->remove(serv);
            }
            if (g) {
                g->add(serv);
                serv->setGroup(g);
            } else {
                logWarn("redis cluster update can't find group for master %.*s %.*s",
                        serv->name().length(), serv->name().data(),
                        serv->addr().length(), serv->addr().data());
            }
        } else if (serv->role() == Server::Slave) {
            ServerGroup* g = getGroup(serv->masterName());
            if (serv->group() && serv->group() != g) {
                serv->group()->remove(serv);
                serv->setGroup(nullptr);
            }
            if (g) {
                g->add(serv);
                serv->setGroup(g);
            } else {
                logWarn("redis cluster update can't find master %.*s for slave %.*s %.*s",
                        serv->masterName().length(), serv->masterName().data(),
                        serv->name().length(), serv->name().data(),
                        serv->addr().length(), serv->addr().data());
            }
        } else {
            logWarn("redis cluster update server %s %s role unknown",
                    serv->name().data(), serv->addr().data());
            if (ServerGroup* g = serv->group()) {
                g->remove(serv);
            }
        }
    }
}

