/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <ctype.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include "LogFileSink.h"
#include "ServerPool.h"
#include "Conf.h"

using std::string;

// [password@]addr role
bool ServerConf::parse(ServerConf& s, const char* str)
{
    const char* p = strchr(str, '@');
    if (p) {
        s.password.assign(str, p);
        str = p + 1;
    } else {
        s.password.clear();
    }
    for (p = str; *p != '\0'; ++p) {
        if (isspace(*p)) {
            break;
        }
    }
    s.addr.assign(str, p);
    return !s.addr.empty();
}

void CustomCommandConf::init(CustomCommandConf&c, const char* name, const int type) {
    c.name = name;
    c.cmd.type = (Command::Type)type;
    c.cmd.name = c.name.c_str();
    c.cmd.minArgs = 1;
    c.cmd.maxArgs = 1;
    c.cmd.mode = Command::Write;
}

Conf::Conf():
    mBind("0.0.0.0:7617"),
    mWorkerThreads(1),
    mMaxMemory(0),
    mClientTimeout(0),
    mBufSize(0),
    mLogRotateSecs(0),
    mLogRotateBytes(0),
    mAllowMissLog(true),
    mServerPoolType(ServerPool::Unknown)
{
    mLogSample[LogLevel::Verb] = 0;
    mLogSample[LogLevel::Debug] = 0;
    mLogSample[LogLevel::Info] = 0;
    mLogSample[LogLevel::Notice] = 1;
    mLogSample[LogLevel::Warn] = 1;
    mLogSample[LogLevel::Error] = 1;
}

Conf::~Conf()
{
}

static void printHelp(const char* name)
{
    fprintf(stderr,
        "Usage: \n"
        "   %s <conf-file> [options]\n"
        "   %s -h or --help\n"
        "   %s -v or --version\n"
        "\n"
        "Options:\n"
        "   --Name=name        set current service name\n"
        "   --Bind=addr        set bind address, eg:127.0.0.1:7617, /tmp/predixy\n"
        "   --WorkerThreads=N  set worker threads\n"
        "   --LocalDC=dc       set local dc\n"
        ,name, name, name
        );
}

static void printVersion(const char* name)
{
    fprintf(stderr, "%s %s-%s\n", name, _PREDIXY_NAME_, _PREDIXY_VERSION_);
}

#define GetVal(s, p) \
 (strncasecmp(s, p, sizeof(p) - 1) == 0 ? s + sizeof(p) - 1 : nullptr)

bool Conf::init(int argc, char* argv[])
{
    if (argc < 2) {
        printHelp(argv[0]);
        Throw(InvalidStartArg, "start %s arguments invalid", argv[0]);
    } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printHelp(argv[0]);
        return false;
    } else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0){
        printVersion(argv[0]);
        return false;
    }
    ConfParser p;
    ConfParser::Node* n = p.load(argv[1]);
    setGlobal(n);
    for (int i = 2; i < argc; ++i) {
        if (char* v = GetVal(argv[i], "--Name=")) {
            mName = v;
        } else if (char* v = GetVal(argv[i], "--Bind=")) {
            mBind = v;
        } else if (char* v = GetVal(argv[i], "--WorkerThreads=")) {
            mWorkerThreads = atoi(v);
        } else if (char* v = GetVal(argv[i], "--LocalDC=")) {
            mLocalDC = v;
        } else {
            printHelp(argv[0]);
            Throw(InvalidStartArg, "invalid argument \"%s\"", argv[i]);
        }
    }
    check();
    return true;
}

void Conf::setGlobal(const ConfParser::Node* node)
{
    const ConfParser::Node* authority = nullptr;
    const ConfParser::Node* clusterServerPool = nullptr;
    const ConfParser::Node* sentinelServerPool = nullptr;
    const ConfParser::Node* dataCenter = nullptr;
    for (auto p = node; p; p = p->next) {
        if (setStr(mName, "Name", p)) {
        } else if (setStr(mBind, "Bind", p)) {
        } else if (setStr(mLocalDC, "LocalDC", p)) {
#ifdef _PREDIXY_SINGLE_THREAD_
        } else if (setInt(mWorkerThreads, "WorkerThreads", p, 1, 1)) {
#else
        } else if (setInt(mWorkerThreads, "WorkerThreads", p, 1)) {
#endif
        } else if (setMemory(mMaxMemory, "MaxMemory", p)) {
        } else if (setLong(mClientTimeout, "ClientTimeout", p, 0)) {
            mClientTimeout *= 1000000;
        } else if (setInt(mBufSize, "BufSize", p, Const::MinBufSize + sizeof(Buffer))) {
            mBufSize -= sizeof(Buffer);
        } else if (setStr(mLog, "Log", p)) {
        } else if (strcasecmp(p->key.c_str(), "LogRotate") == 0) {
            if (!LogFileSink::parseRotate(p->val.c_str(), mLogRotateSecs, mLogRotateBytes)) {
                Throw(InvalidValue, "%s:%d invalid LogRotate \"%s\"",
                        p->file, p->line, p->val.c_str());
            }
        } else if (setBool(mAllowMissLog, "AllowMissLog", p)) {
        } else if (setInt(mLogSample[LogLevel::Verb], "LogVerbSample", p)) {
        } else if (setInt(mLogSample[LogLevel::Debug], "LogDebugSample", p)) {
        } else if (setInt(mLogSample[LogLevel::Info], "LogInfoSample", p)) {
        } else if (setInt(mLogSample[LogLevel::Notice], "LogNoticeSample", p)) {
        } else if (setInt(mLogSample[LogLevel::Warn], "LogWarnSample", p)) {
        } else if (setInt(mLogSample[LogLevel::Error], "LogErrorSample", p)) {
        } else if (strcasecmp(p->key.c_str(), "LatencyMonitor") == 0) {
            mLatencyMonitors.push_back(LatencyMonitorConf{});
            setLatencyMonitor(mLatencyMonitors.back(), p);
        } else if (strcasecmp(p->key.c_str(), "Authority") == 0) {
            authority = p;
        } else if (strcasecmp(p->key.c_str(), "ClusterServerPool") == 0) {
            clusterServerPool = p;
        } else if (strcasecmp(p->key.c_str(), "SentinelServerPool") == 0) {
            sentinelServerPool = p;
        } else if (strcasecmp(p->key.c_str(), "DataCenter") == 0) {
            dataCenter = p;
        } else if (strcasecmp(p->key.c_str(), "CustomCommand") == 0) {
            setCustomCommand(p);
        } else {
            Throw(UnknownKey, "%s:%d unknown key %s", p->file, p->line, p->key.c_str());
        }
    }
    if (authority) {
        setAuthority(authority);
    }
    if (clusterServerPool && sentinelServerPool) {
        Throw(LogicError, "Can't define ClusterServerPool and SentinelServerPool at the same time");
    } else if (clusterServerPool) {
        setClusterServerPool(clusterServerPool);
        mServerPoolType = ServerPool::Cluster;
    } else if (sentinelServerPool) {
        setSentinelServerPool(sentinelServerPool);
        mServerPoolType = ServerPool::Sentinel;
    } else {
        Throw(LogicError, "Must define a server pool");
    }
    if (dataCenter) {
        setDataCenter(dataCenter);
    }
}

static void setKeyPrefix(std::vector<std::string>& dat, const std::string& v)
{
    const char* p = v.c_str();
    const char* s = nullptr;
    do {
        if (*p == ' ' || *p == '\t' || *p == '\0') {
            if (s) {
                dat.push_back(std::string(s, p - s));
                s = nullptr;
            }
        } else if (!s) {
            s = p;
        }
    } while (*p++);
}

void Conf::setAuthority(const ConfParser::Node* node)
{
    if (!node->sub) {
        Throw(InvalidValue, "%s:%d Authority require scope value", node->file, node->line);
    }
    for (auto p = node->sub; p; p = p->next) {
        if (strcasecmp(p->key.c_str(), "Auth") != 0) {
            Throw(InvalidValue, "%s:%d Authority allow only Auth element", p->file, p->line);
        }
        if (!p->sub) {
            Throw(InvalidValue, "%s:%d Auth require scope value", p->file, p->line);
        }
        mAuthConfs.push_back(AuthConf{});
        auto& c = mAuthConfs.back();
        c.password = p->val;
        for (auto n = p->sub; n; n = n->next) {
            const std::string& k = n->key;
            const std::string& v = n->val;
            if (strcasecmp(k.c_str(), "Mode") == 0) {
                if (strcasecmp(v.c_str(), "read") == 0) {
                    c.mode = Command::Read;
                } else if (strcasecmp(v.c_str(), "write") == 0) {
                    c.mode = Command::Write|Command::Read;
                } else if (strcasecmp(v.c_str(), "admin") == 0) {
                    c.mode = Command::Write|Command::Read|Command::Admin;
                } else {
                    Throw(InvalidValue, "%s:%d Auth Mode must be read or write", n->file, n->line);
                }
            } else if (strcasecmp(k.c_str(), "KeyPrefix") == 0) {
                setKeyPrefix(c.keyPrefix, v);
            } else if (strcasecmp(k.c_str(), "ReadKeyPrefix") == 0) {
                setKeyPrefix(c.readKeyPrefix, v);
            } else if (strcasecmp(k.c_str(), "WriteKeyPrefix") == 0) {
                setKeyPrefix(c.writeKeyPrefix, v);
            } else {
                Throw(UnknownKey, "%s:%d unknown key %s", n->file, n->line, k.c_str());
            }
        }
    }
}

bool Conf::setServerPool(ServerPoolConf& sp, const ConfParser::Node* p)
{
    if (setStr(sp.password, "Password", p)) {
        return true;
    } else if (setInt(sp.masterReadPriority, "MasterReadPriority", p, 0, 100)) {
        return true;
    } else if (setInt(sp.staticSlaveReadPriority, "StaticSlaveReadPriority", p, 0, 100)) {
        return true;
    } else if (setInt(sp.dynamicSlaveReadPriority, "DynamicSlaveReadPriority", p, 0, 100)) {
        return true;
    } else if (setDuration(sp.refreshInterval, "RefreshInterval", p)) {
        return true;
    } else if (setDuration(sp.serverTimeout, "ServerTimeout", p)) {
        return true;
    } else if (setInt(sp.serverFailureLimit, "ServerFailureLimit", p, 1)) {
        return true;
    } else if (setDuration(sp.serverRetryTimeout, "ServerRetryTimeout", p)) {
        return true;
    } else if (setInt(sp.keepalive, "KeepAlive", p, 0)) {
        return true;
    } else if (setInt(sp.databases, "Databases", p, 1, 128)) {
        return true;
    }
    return false;
}

void Conf::setClusterServerPool(const ConfParser::Node* node)
{
    if (!node->sub) {
        Throw(InvalidValue, "%s:%d ClusterServerPool require scope value", node->file, node->line);
    }
    for (auto p = node->sub; p; p = p->next) {
        if (setServerPool(mClusterServerPool, p)) {
        } else if (setServers(mClusterServerPool.servers, "Servers", p)) {
        } else {
            Throw(UnknownKey, "%s:%d unknown key %s",
                    p->file, p->line, p->key.c_str());
        }
    }
    if (mClusterServerPool.databases != 1) {
        Throw(InvalidValue, "ClusterServerPool Databases must be 1");
    }
    if (mClusterServerPool.servers.empty()) {
        Throw(LogicError, "ClusterServerPool no server");
    }
}

void Conf::setSentinelServerPool(const ConfParser::Node* node)
{
    if (!node->sub) {
        Throw(InvalidValue, "%s:%d SentinelServerPool require scope value", node->file, node->line);
    }
    mSentinelServerPool.hashTag[0] = '\0';
    mSentinelServerPool.hashTag[1] = '\0';
    for (auto p = node->sub; p; p = p->next) {
        if (setServerPool(mSentinelServerPool, p)) {
        } else if (strcasecmp(p->key.c_str(), "Distribution") == 0) {
            mSentinelServerPool.dist = Distribution::parse(p->val.c_str());
            if (mSentinelServerPool.dist == Distribution::None) {
                Throw(InvalidValue, "%s:%d unknown Distribution", p->file, p->line);
            }
        } else if (strcasecmp(p->key.c_str(), "Hash") == 0) {
            mSentinelServerPool.hash = Hash::parse(p->val.c_str());
            if (mSentinelServerPool.hash == Hash::None) {
                Throw(InvalidValue, "%s:%d unknown Hash", p->file, p->line);
            }
        } else if (strcasecmp(p->key.c_str(), "HashTag") == 0) {
            if (p->val.empty()) {
                mSentinelServerPool.hashTag[0] = '\0';
                mSentinelServerPool.hashTag[1] = '\0';
            } else if (p->val.size() == 2) {
                mSentinelServerPool.hashTag[0] = p->val[0];
                mSentinelServerPool.hashTag[1] = p->val[1];
            } else {
                Throw(InvalidValue, "%s:%d HashTag invalid", p->file, p->line);
            }
        } else if (setServers(mSentinelServerPool.sentinels, "Sentinels", p)) {
            mSentinelServerPool.sentinelPassword = p->val;
        } else if (strcasecmp(p->key.c_str(), "Group") == 0) {
            mSentinelServerPool.groups.push_back(ServerGroupConf{p->val});
            if (p->sub) {
                auto& g = mSentinelServerPool.groups.back();
                setServers(g.servers, "Group", p);
            }
        } else {
            Throw(UnknownKey, "%s:%d unknown key %s",
                    p->file, p->line, p->key.c_str());
        }
    }
    if (mSentinelServerPool.sentinels.empty()) {
        Throw(LogicError, "SentinelServerPool no sentinel server");
    }
    if (mSentinelServerPool.groups.empty()) {
        Throw(LogicError, "SentinelServerPool no server group");
    }
    if (mSentinelServerPool.groups.size() > 1) {
        if (mSentinelServerPool.dist == Distribution::None) {
            Throw(LogicError, "SentinelServerPool must define Dsitribution in multi groups");
        }
        if (mSentinelServerPool.hash == Hash::None) {
            Throw(LogicError, "SentinelServerPool must define Hash in multi groups");
        }
    }
}

void Conf::setDataCenter(const ConfParser::Node* node)
{
    if (!node->sub) {
        Throw(InvalidValue, "%s:%d DataCenter require scope value", node->file, node->line);
    }
    for (auto p = node->sub; p; p = p->next) {
        if (strcasecmp(p->key.c_str(), "DC") != 0) {
            Throw(InvalidValue, "%s:%d DataCenter allow only DC element", p->file, p->line);
        }
        mDCConfs.push_back(DCConf{});
        auto& dc = mDCConfs.back();
        dc.name = p->val;
        setDC(dc, p);
    }
}

void Conf::setCustomCommand(const ConfParser::Node* node)
{
    if (!node->sub) {
       Throw(InvalidValue, "%s:%d CustomCommand require scope value", node->file, node->line);
    }
    for (auto p = node->sub; p; p = p->next) {
       if (Command::Sentinel >= Command::AvailableCommands) {
          Throw(InvalidValue, "%s:%d Too many custom commands", node->file, node->line);
       }
       mCustomCommands.push_back(CustomCommandConf{});
       auto& cc = mCustomCommands.back();
       CustomCommandConf::init(cc, p->key.c_str(), Command::Sentinel);
       auto s = p->sub;
       if (!s) {
           Throw(InvalidValue, "%s:%d CustomCommand.Command require scope value",
                  node->file, node->line);
       }
       for (;s ; s = s->next) {
           setInt(cc.cmd.minArgs, "minArgs", s, 1);
           setInt(cc.cmd.maxArgs, "maxArgs", s, 1, 9999);
           setCommandMode(cc.cmd.mode, "mode", s);
       }
       Command::addCustomCommand(&cc.cmd);
    }
}

bool Conf::setCommandMode(int& mode, const char* name, const ConfParser::Node* n, const int defaultMode)
{
    if (strcasecmp(name, n->key.c_str()) == 0) {
        if (n->val.size() == 0) {
            mode = defaultMode;
        } else {
            mode = Command::Unknown;
        }
        std::string mask;
        std::istringstream is(n->val);
        while (std::getline(is, mask, ',')) {
            if ((strcasecmp(mask.c_str(), "Write") == 0)) {
               mode |= Command::Write;
            } else if ((strcasecmp(mask.c_str(), "Read") == 0)) {
               mode |= Command::Read;
            } else if ((strcasecmp(mask.c_str(), "Admin") == 0)) {
               mode |= Command::Admin;
            } else if ((strcasecmp(mask.c_str(), "Private") == 0)) {
               mode |= Command::Private;
            } else if ((strcasecmp(mask.c_str(), "NoKey") == 0)) {
               mode |= Command::NoKey;
            } else if ((strcasecmp(mask.c_str(), "MultiKey") == 0)) {
               mode |= Command::MultiKey;
            } else if ((strcasecmp(mask.c_str(), "SMultiKey") == 0)) {
               mode |= Command::SMultiKey;
            } else if ((strcasecmp(mask.c_str(), "MultiKeyVal") == 0)) {
               mode |= Command::MultiKeyVal;
            } else if ((strcasecmp(mask.c_str(), "KeyAt2") == 0)) {
               mode |= Command::KeyAt2;
            } else if ((strcasecmp(mask.c_str(), "KeyAt3") == 0)) {
               mode |= Command::KeyAt3;
            } else {
                Throw(InvalidValue, "%s:%d %s %s is not an mode",
                    n->file, n->line, name, n->val.c_str());
            }
        }
        return true;
    }
    return false;
}

void Conf::setDC(DCConf& dc, const ConfParser::Node* node)
{
    if (!node->sub) {
        Throw(InvalidValue, "%s:%d DC require scope value",
                node->file, node->line);
    }
    for (auto p = node->sub; p; p = p->next) {
        if (strcasecmp(p->key.c_str(), "AddrPrefix") == 0) {
            if (!p->sub) {
                Throw(InvalidValue, "%s:%d AddrPrefix require scope value",
                                    p->file, p->line);
            }
            for (auto n = p->sub; n; n = n->next) {
                if (strcmp(n->key.c_str(), "+") == 0) {
                    dc.addrPrefix.push_back(n->val);
                } else {
                    Throw(InvalidValue, "%s:%d invalid AddrPrefix",
                            n->file, n->line);
                }
            }
        } else if (strcasecmp(p->key.c_str(), "ReadPolicy") == 0) {
            if (!p->sub) {
                Throw(InvalidValue, "%s:%d ReadPolicy require scope value",
                                    p->file, p->line);
            }
            for (auto n = p->sub; n; n = n->next) {
                dc.readPolicy.push_back(ReadPolicyConf{});
                setReadPolicy(dc.readPolicy.back(), n);
            }
        } else {
            Throw(UnknownKey, "%s:%d unknown key %s",
                    p->file, p->line, p->key.c_str());
        }
    }
}

void Conf::setReadPolicy(ReadPolicyConf& c, const ConfParser::Node* n)
{
    c.name = n->key;
    int num = sscanf(n->val.c_str(), "%d %d", &c.priority, &c.weight);
    if (num == 1) {
        c.weight = 1;
    } else if (num != 2) {
        Throw(InvalidValue, "%s:%d invalid ReadPolicy \"%s\"",
                n->file, n->line, n->val.c_str());
    }
}

void Conf::setLatencyMonitor(LatencyMonitorConf& m, const ConfParser::Node* node)
{
    if (!node->sub) {
        Throw(InvalidValue, "%s:%d LatencyMonitor require scope value",
                node->file, node->line);
    }
    m.name = node->val;
    for (auto p = node->sub; p; p = p->next) {
        if (strcasecmp(p->key.c_str(), "Commands") == 0) {
            for (auto n = p->sub; n; n = n->next) {
                bool add = true;
                if (strcmp(n->key.c_str(), "+") == 0) {
                } else if (strcmp(n->key.c_str(), "-") == 0) {
                    add = false;
                } else {
                    Throw(InvalidValue, "%s:%d invalid Command assign",
                            n->file, n->line);
                }
                const char* v = n->val.c_str();
                if (strcasecmp(v, "all") == 0) {
                    add ? m.cmds.set() : m.cmds.reset();
                } else if (auto c = Command::find(v)) {
                    add ? m.cmds.set(c->type) : m.cmds.reset(c->type);
                } else {
                    Throw(InvalidValue, "%s:%d unknown Command \"%s\"",
                            n->file, n->line, v);
                }
            }
        } else if (strcasecmp(p->key.c_str(), "TimeSpan") == 0) {
            long span = 0;
            for (auto n = p->sub; n; n = n->next) {
                if (setLong(span, "+", n, span + 1)) {
                    m.timeSpan.push_back(span);
                } else {
                    Throw(InvalidValue, "%s:%d invalid TimeSpan",
                            n->file, n->line);
                }
            }
        }
    }
}

void Conf::check()
{
#ifdef _PREDIXY_SINGLE_THREAD_
    if (mWorkerThreads != 1) {
        Throw(InvalidValue, "WorkerThreads must be 1 in single thread mode");
    }
#else
    if (mWorkerThreads < 1) {
        Throw(InvalidValue, "WorkerThreads must >= 1");
    }
#endif
    if (mLocalDC.empty()) {
        if (!mDCConfs.empty()) {
            Throw(LogicError, "conf define DataCenter but not define LocalDC");
        }
    } else {
        bool found = false;
        for (auto& dc : mDCConfs) {
            if (dc.name == mLocalDC) {
                found = true;
                break;
            }
        }
        if (!found) {
            Throw(LogicError, "LocalDC \"%s\" no exists in DataCenter",
                    mLocalDC.c_str());
        }
        for (auto& dc : mDCConfs) {
            for (auto& rp : dc.readPolicy) {
                found = false;
                for (auto& i :mDCConfs) {
                    if (rp.name == i.name) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    Throw(LogicError, "DC \"%s\" ReadPolicy \"%s\" no eixsts in DataCenter", dc.name.c_str(), rp.name.c_str());
                }
            }
        }
    }
}

bool Conf::setStr(std::string& attr, const char* name, const ConfParser::Node* n)
{
    if (strcasecmp(name, n->key.c_str()) == 0) {
        attr = n->val;
        return true;
    }
    return false;
}

bool Conf::setInt(int& attr, const char* name, const ConfParser::Node* n, int lower, int upper)
{
    if (strcasecmp(name, n->key.c_str()) == 0) {
        if (sscanf(n->val.c_str(), "%d", &attr) != 1) {
            Throw(InvalidValue, "%s:%d %s %s is not an integer",
                  n->file, n->line, name, n->val.c_str());
        }
        if (attr < lower || attr > upper) {
            Throw(InvalidValue, "%s:%d %s %s not in range [%d, %d]",
                n->file, n->line, name, n->val.c_str(), lower, upper);
        }
        return true;
    }
    return false;
}

bool Conf::setLong(long& attr, const char* name, const ConfParser::Node* n, long lower, long upper)
{
    if (strcasecmp(name, n->key.c_str()) == 0) {
        if (sscanf(n->val.c_str(), "%ld", &attr) != 1) {
            Throw(InvalidValue, "%s:%d %s %s is not an long integer",
                  n->file, n->line, name, n->val.c_str());
        }
        if (attr < lower || attr > upper) {
            Throw(InvalidValue, "%s:%d %s %s not in range [%ld, %ld]",
                n->file, n->line, name, n->val.c_str(), lower, upper);
        }
        return true;
    }
    return false;
}

bool Conf::setBool(bool& attr, const char* name, const ConfParser::Node* n)
{
    if (strcasecmp(name, n->key.c_str()) == 0) {
        if (strcasecmp(n->val.c_str(), "true") == 0) {
            attr = true;
        } else if (strcasecmp(n->val.c_str(), "false") == 0) {
            attr = false;
        } else {
            Throw(InvalidValue, "%s:%d %s expect true or false",
                    n->file, n->line, name);
        }
        return true;
    }
    return false;
}

bool Conf::parseMemory(long& m, const char* str)
{
    char u[4];
    int c = sscanf(str, "%ld%3s", &m, u);
    if (c == 2 && m > 0) {
        if (strcasecmp(u, "B") == 0) {
        } else if (strcasecmp(u, "K") == 0 || strcasecmp(u, "KB") == 0) {
            m <<= 10;
        } else if (strcasecmp(u, "M") == 0 || strcasecmp(u, "MB") == 0) {
            m <<= 20;
        } else if (strcasecmp(u, "G") == 0 || strcasecmp(u, "GB") == 0) {
            m <<= 30;
        } else {
            return false;
        }
    } else if (c != 1) {
        return false;
    }
    return m >= 0;
}

bool Conf::parseDuration(long& v, const char* str)
{
    char u[4];
    int c = sscanf(str, "%ld%3s", &v, u);
    if (c == 2 && v > 0) {
        if (strcasecmp(u, "s") == 0) {
            v *= 1000000;
        } else if (strcasecmp(u, "m") == 0 || strcasecmp(u, "ms") == 0) {
            v *= 1000;
        } else if (strcasecmp(u, "u") == 0 || strcasecmp(u, "us") == 0) {
        } else {
            return false;
        }
    } else if (c == 1) {
        v *= 1000000;
    } else {
        return false;
    }
    return v >= 0;
}

bool Conf::setMemory(long& m, const char* name, const ConfParser::Node* n)
{
    if (strcasecmp(name, n->key.c_str()) != 0) {
        return false;
    }
    if (!parseMemory(m, n->val.c_str())) {
        Throw(InvalidValue, "%s:%d %s invalid memory value \"%s\"",
                n->file, n->line, name, n->val.c_str());
    }
    return true;
}

bool Conf::setDuration(long& v, const char* name, const ConfParser::Node* n)
{
    if (strcasecmp(name, n->key.c_str()) != 0) {
        return false;
    }
    if (!parseDuration(v, n->val.c_str())) {
        Throw(InvalidValue, "%s:%d %s invalid duration value \"%s\"",
                n->file, n->line, name, n->val.c_str());
    }
    return true;
}

bool Conf::setServers(std::vector<ServerConf>& servs, const char* name, const ConfParser::Node* p)
{
    if (strcasecmp(p->key.c_str(), name) != 0) {
        return false;
    }
    if (!p->sub) {
        Throw(InvalidValue, "%s:%d %s require scope value", p->file, p->line, name);
    }
    for (auto n = p->sub; n; n = n->next) {
        if (strcasecmp(n->key.c_str(), "+") == 0) {
            ServerConf s;
            if (ServerConf::parse(s, n->val.c_str())) {
                servs.push_back(s);
            } else {
                Throw(InvalidValue, "%s:%d invalid server define %s",
                        n->file, n->line, n->val.c_str());
            }
        } else {
            Throw(InvalidValue, "%s:%d invalid server define", n->file, n->line);
        }
    }
    return true;
}
