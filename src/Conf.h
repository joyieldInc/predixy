/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_CONF_H_
#define _PREDIXY_CONF_H_

#include <limits.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <bitset>
#include "Predixy.h"
#include "Distribution.h"
#include "ConfParser.h"
#include "Auth.h"
#include "Command.h"
#include "Enums.h"

struct AuthConf
{
    std::string password;
    int mode; //Command::Mode
    std::vector<std::string> keyPrefix;
    std::vector<std::string> readKeyPrefix;
    std::vector<std::string> writeKeyPrefix;
};

struct ServerConf
{
    std::string password;
    std::string addr;

    static bool parse(ServerConf& s, const char* str);
};

struct ServerGroupConf
{
    std::string name;
    std::vector<ServerConf> servers;
};

struct ServerPoolConf
{
    std::string password;
    int masterReadPriority = 50;
    int staticSlaveReadPriority = 0;
    int dynamicSlaveReadPriority = 0;
    long refreshInterval = 1000000;    //us
    long serverTimeout = 0; //us
    int serverFailureLimit = 10;
    long serverRetryTimeout = 1000000; //us
    int keepalive = 0; //seconds
    int databases = 1;
};

struct ClusterServerPoolConf : public ServerPoolConf
{
    std::vector<ServerConf> servers;
};

struct StandaloneServerPoolConf : public ServerPoolConf
{
    ServerPoolRefreshMethod refreshMethod = ServerPoolRefreshMethod::None;
    Distribution dist = Distribution::None;
    Hash hash = Hash::None;
    char hashTag[2];
    std::string sentinelPassword;
    std::vector<ServerConf> sentinels;
    std::vector<ServerGroupConf> groups;
};

struct ReadPolicyConf
{
    std::string name;
    int priority;
    int weight;
};

struct DCConf
{
    std::string name;
    std::vector<std::string> addrPrefix;
    std::vector<ReadPolicyConf> readPolicy;
};

struct LatencyMonitorConf
{
    std::string name;
    std::bitset<Command::AvailableCommands> cmds;
    std::vector<long> timeSpan;//us
};

struct CustomCommandConf
{
    std::string name;
    int minArgs;
    int maxArgs;
    int mode;

    static void init(CustomCommandConf &c, const char* name);
};

class Conf
{
public:
    DefException(InvalidStartArg);
    DefException(UnknownKey);
    DefException(InvalidValue);
    DefException(LogicError);
public:
    Conf();
    ~Conf();
    bool init(int argc, char* argv[]);
    const char* name() const
    {
        return mName.c_str();
    }
    const char* bind() const
    {
        return mBind.c_str();
    }
    int workerThreads() const
    {
        return mWorkerThreads;
    }
    long maxMemory() const
    {
        return mMaxMemory;
    }
    void setClientTimeout(long v)
    {
        mClientTimeout = v;
    }
    long clientTimeout() const
    {
        return mClientTimeout;
    }
    int bufSize() const
    {
        return mBufSize;
    }
    const std::vector<AuthConf>& authConfs() const
    {
        return mAuthConfs;
    }
    const char* log() const
    {
        return mLog.c_str();
    }
    int logRotateSecs() const
    {
        return mLogRotateSecs;
    }
    long logRotateBytes() const
    {
        return mLogRotateBytes;
    }
    bool allowMissLog() const
    {
        return mAllowMissLog;
    }
    int logSample(LogLevel::Type lvl) const
    {
        return mLogSample[lvl];
    }
    int serverPoolType() const
    {
        return mServerPoolType;
    }
    const ClusterServerPoolConf& clusterServerPool() const
    {
        return mClusterServerPool;
    }
    const StandaloneServerPoolConf& standaloneServerPool() const
    {
        return mStandaloneServerPool;
    }
    const std::string& localDC() const
    {
        return mLocalDC;
    }
    const std::vector<DCConf>& dcConfs() const
    {
        return mDCConfs;
    }
    const std::vector<LatencyMonitorConf>& latencyMonitors() const
    {
        return mLatencyMonitors;
    }
public:
    static bool parseMemory(long& m, const char* str);
    static bool parseDuration(long& v, const char* str);
private:
    void setGlobal(const ConfParser::Node* node);
    void setAuthority(const ConfParser::Node* node);
    void setClusterServerPool(const ConfParser::Node* node);
    void setStandaloneServerPool(const ConfParser::Node* node);
    void setDataCenter(const ConfParser::Node* node);
    void check();
    bool setServerPool(ServerPoolConf& sp, const ConfParser::Node* n);
    bool setStr(std::string& attr, const char* name, const ConfParser::Node* n);
    bool setInt(int& attr, const char* name, const ConfParser::Node* n, int lower = INT_MIN, int upper = INT_MAX);
    bool setLong(long& attr, const char* name, const ConfParser::Node* n, long lower = LONG_MIN, long upper = LONG_MAX);
    bool setBool(bool& attr, const char* name, const ConfParser::Node* n);
    bool setMemory(long& mem, const char* name, const ConfParser::Node* n);
    bool setDuration(long& v, const char* name, const ConfParser::Node* n);
    bool setServers(std::vector<ServerConf>& servs, const char* name, const ConfParser::Node* n);
    void setDC(DCConf& dc, const ConfParser::Node* n);
    void setReadPolicy(ReadPolicyConf& c, const ConfParser::Node* n);
    void setLatencyMonitor(LatencyMonitorConf& m, const ConfParser::Node* n);
    void setCustomCommand(const ConfParser::Node* n);
    bool setCommandMode(int& mode, const char* name, const ConfParser::Node* n, const int defaultMode = Command::Write);
private:
    std::string mName;
    std::string mBind;
    int mWorkerThreads;
    long mMaxMemory;
    long mClientTimeout; //us
    int mBufSize;
    std::string mLog;
    int mLogRotateSecs;
    long mLogRotateBytes;
    bool mAllowMissLog;
    int mLogSample[LogLevel::Sentinel];
    std::vector<AuthConf> mAuthConfs;
    int mServerPoolType;
    ClusterServerPoolConf mClusterServerPool;
    StandaloneServerPoolConf mStandaloneServerPool;
    std::vector<DCConf> mDCConfs;
    std::string mLocalDC;
    std::vector<LatencyMonitorConf> mLatencyMonitors;
    std::vector<CustomCommandConf> mCustomCommands;
};


#endif
