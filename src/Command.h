/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#ifndef _PREDIXY_COMMAND_H_
#define _PREDIXY_COMMAND_H_

#include <unordered_map>
#include "Exception.h"
#include "HashFunc.h"

struct CustomCommandConf;

class Command
{
public:
    DefException(InitFail);
    enum Type
    {
        None = 0,
        Ping,
        PingServ,
        Echo,
        Auth,
        AuthServ,
        Select,
        SelectServ,
        Quit,

        SentinelSentinels,
        SentinelGetMaster,
        SentinelSlaves,

        Cmd,
        Info,
        Config,

        Cluster,
        ClusterNodes,
        Asking,
        Readonly,

        Watch,
        Unwatch,
        UnwatchServ,
        Multi,
        Exec,
        Discard,
        DiscardServ,

        Eval,
        Evalsha,
        Script,
        ScriptLoad,

        Del,
        Dump,
        Exists,
        Expire,
        Expireat,
        Move,
        Persist,
        Pexpire,
        Pexpireat,
        Pttl,
        Randomkey,
        Rename,
        Renamenx,
        Restore,
        Sort,
        Touch,
        Ttl,
        TypeCmd,
        Unlink,
        Scan,
        Append,
        Bitcount,
        Bitfield,
        Bitop,
        Bitpos,
        Decr,
        Decrby,
        Get,
        Getbit,
        Getrange,
        Getset,
        Incr,
        Incrby,
        Incrbyfloat,
        Mget,
        Mset,
        Msetnx,
        Psetex,
        Set,
        Setbit,
        Setex,
        Setnx,
        Setrange,
        Strlen,

        Hdel,
        Hexists,
        Hget,
        Hgetall,
        Hincrby,
        Hincrbyfloat,
        Hkeys,
        Hlen,
        Hmget,
        Hmset,
        Hscan,
        Hset,
        Hsetnx,
        Hstrlen,
        Hvals,

        Blpop,
        Brpop,
        Brpoplpush,
        Lindex,
        Linsert,
        Llen,
        Lpop,
        Lpush,
        Lpushx,
        Lrange,
        Lrem,
        Lset,
        Ltrim,
        Rpop,
        Rpoplpush,
        Rpush,
        Rpushx,

        Sadd,
        Scard,
        Sdiff,
        Sdiffstore,
        Sinter,
        Sinterstore,
        Sismember,
        Smembers,
        Smove,
        Spop,
        Srandmember,
        Srem,
        Sscan,
        Sunion,
        Sunionstore,

        Zadd,
        Zcard,
        Zcount,
        Zincrby,
        Zinterstore,
        Zlexcount,
        Zpopmax,
        Zpopmin,
        Zrange,
        Zrangebylex,
        Zrangebyscore,
        Zrank,
        Zrem,
        Zremrangebylex,
        Zremrangebyrank,
        Zremrangebyscore,
        Zrevrange,
        Zrevrangebylex,
        Zrevrangebyscore,
        Zrevrank,
        Zscan,
        Zscore,
        Zunionstore,

        Pfadd,
        Pfcount,
        Pfmerge,

        Geoadd,
        Geodist,
        Geohash,
        Geopos,
        Georadius,
        Georadiusbymember,

        Psubscribe,
        Publish,
        Pubsub,
        Punsubscribe,
        Subscribe,
        Unsubscribe,
        SubMsg,

        MaxCommands,
        MaxCustomCommands = 16,
        AvailableCommands = MaxCommands + MaxCustomCommands,
    };
    enum Mode
    {
        Unknown     = 0,
        Read        = 1<<0,
        Write       = 1<<1,
        Admin       = 1<<2,
        Private     = 1<<3,   //require private connection
        NoKey       = 1<<4,
        MultiKey    = 1<<5,
        SMultiKey   = 1<<6,
        MultiKeyVal = 1<<7,
        KeyAt2      = 1<<8,
        KeyAt3      = 1<<9,
        SubCmd      = 1<<10,
        Inner       = 1<<11    //proxy use only
    };
    static const int AuthMask = Read|Write|Admin;
    static const int KeyMask = NoKey|MultiKey|SMultiKey|MultiKeyVal|KeyAt2|KeyAt3;
public:
    Type type;
    const char* name;
    int minArgs;
    int maxArgs;
    int mode;

    bool isMultiKey() const
    {
        return mode & MultiKey;
    }
    bool isSMultiKey() const
    {
        return mode & SMultiKey;
    }
    bool isMultiKeyVal() const
    {
        return mode & MultiKeyVal;
    }
    bool isAnyMulti() const
    {
        return mode & (MultiKey|SMultiKey|MultiKeyVal);
    }
    static void init();
    static const Command& get(Type type)
    {
        return CmdPool[type];
    }
    static const Command* iter(int& cursor)
    {
        if (cursor < Sentinel) {
            return &CmdPool[cursor++];
        }
        return nullptr;
    }
    static const Command* find(const String& cmd)
    {
        auto it = CmdMap.find(cmd);
        return it == CmdMap.end() ? nullptr : it->second;
    }
    static void addCustomCommand(const CustomCommandConf& pc);
    static int Sentinel;
private:
    static const int MaxArgs = 100000000;
    static Command CmdPool[];
    class H
    {
    public:
        size_t operator()(const String& s) const
        {
            return Hash::crc16(s.data(), s.length());
        }
    };
    typedef std::unordered_map<String, const Command*, H> CommandMap;
    static CommandMap CmdMap;
};

#endif
