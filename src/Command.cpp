/*
 * predixy - A high performance and full features proxy for redis.
 * Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
 * All rights reserved.
 */

#include <string.h>
#include <strings.h>
#include <map>
#include "String.h"
#include "Command.h"
#include "Conf.h"

Command Command::CmdPool[AvailableCommands] = {
    {None,              "",                 0,  MaxArgs,   Read},
    {Ping,              "ping",             1,  2,         Read},
    {PingServ,          "ping",             1,  2,         Inner},
    {Echo,              "echo",             2,  2,         Read},
    {Auth,              "auth",             2,  2,         Read},
    {AuthServ,          "auth",             2,  2,         Inner},
    {Select,            "select",           2,  2,         Read},
    {SelectServ,        "select",           2,  2,         Inner},
    {Quit,              "quit",             1,  MaxArgs,   Read},
    {SentinelSentinels, "sentinel sentinels",3, 3,         Inner},
    {SentinelGetMaster, "sentinel get-m-a..",3, 3,         Inner},
    {SentinelSlaves,    "sentinel slaves",  3,  3,         Inner},
    {Cmd,               "command",          1,  1,         Read},
    {Info,              "info",             1,  4,         Read},
    {Config,            "config",           2,  4,         Admin},
    {Cluster,           "cluster",          2,  2,         Inner},
    {ClusterNodes,      "cluster nodes",    2,  2,         SubCmd|Inner},
    {Asking,            "asking",           1,  1,         Inner},
    {Readonly,          "readonly",         1,  1,         Inner},
    {Watch,             "watch",            2,  MaxArgs,   Write|Private},
    {Unwatch,           "unwatch",          1,  1,         Write},
    {UnwatchServ,       "unwatch",          1,  1,         Inner},
    {Multi,             "multi",            1,  1,         Write|Private|NoKey},
    {Exec,              "exec",             1,  1,         Write|NoKey},
    {Discard,           "discard",          1,  1,         Write|NoKey},
    {DiscardServ,       "discard",          1,  1,         Inner|NoKey},
    {Eval,              "eval",             3,  MaxArgs,   Write|KeyAt3},
    {Evalsha,           "evalsha",          3,  MaxArgs,   Write|KeyAt3},
    {Script,            "script",           2,  MaxArgs,   Write},
    {ScriptLoad,        "script",           3,  3,         Write|SubCmd},
    {Del,               "del",              2,  MaxArgs,   Write|MultiKey},
    {Dump,              "dump",             2,  2,         Read},
    {Exists,            "exists",           2,  MaxArgs,   Read|MultiKey},
    {Expire,            "expire",           3,  3,         Write},
    {Expireat,          "expireat",         3,  3,         Write},
    {Move,              "move",             3,  3,         Write},
    {Persist,           "persist",          2,  2,         Write},
    {Pexpire,           "pexpire",          3,  3,         Write},
    {Pexpireat,         "pexpireat",        3,  3,         Write},
    {Pttl,              "pttl",             2,  2,         Read},
    {Randomkey,         "randomkey",        1,  1,         Read|NoKey},
    {Rename,            "rename",           3,  3,         Write},
    {Renamenx,          "renamenx",         3,  3,         Write},
    {Restore,           "restore",          4,  5,         Write},
    {Sort,              "sort",             2,  MaxArgs,   Write},
    {Touch,             "touch",            2,  MaxArgs,   Write|MultiKey},
    {Ttl,               "ttl",              2,  2,         Read},
    {TypeCmd,           "type",             2,  2,         Read},
    {Unlink,            "unlink",           2,  MaxArgs,   Write|MultiKey},
    {Scan,              "scan",             2,  6,         Read},
    {Append,            "append",           3,  3,         Write},
    {Bitcount,          "bitcount",         2,  4,         Read},
    {Bitfield,          "bitfield",         2,  MaxArgs,   Write},
    {Bitop,             "bitop",            4,  MaxArgs,   Write|KeyAt2},
    {Bitpos,            "bitpos",           3,  5,         Read},
    {Decr,              "decr",             2,  2,         Write},
    {Decrby,            "decrby",           3,  3,         Write},
    {Get,               "get",              2,  2,         Read},
    {Getbit,            "getbit",           3,  3,         Read},
    {Getrange,          "getrange",         4,  4,         Read},
    {Getset,            "getset",           3,  3,         Write},
    {Incr,              "incr",             2,  2,         Write},
    {Incrby,            "incrby",           3,  3,         Write},
    {Incrbyfloat,       "incrbyfloat",      3,  3,         Write},
    {Mget,              "mget",             2,  MaxArgs,   Read|MultiKey},
    {Mset,              "mset",             3,  MaxArgs,   Write|MultiKeyVal},
    {Msetnx,            "msetnx",           3,  MaxArgs,   Write|MultiKeyVal},
    {Psetex,            "psetex",           4,  4,         Write},
    {Set,               "set",              3,  8,         Write},
    {Setbit,            "setbit",           4,  4,         Write},
    {Setex,             "setex",            4,  4,         Write},
    {Setnx,             "setnx",            3,  3,         Write},
    {Setrange,          "setrange",         4,  4,         Write},
    {Strlen,            "strlen",           2,  2,         Read},
    {Hdel,              "hdel",             3,  MaxArgs,   Write},
    {Hexists,           "hexists",          3,  3,         Read},
    {Hget,              "hget",             3,  3,         Read},
    {Hgetall,           "hgetall",          2,  2,         Read},
    {Hincrby,           "hincrby",          4,  4,         Write},
    {Hincrbyfloat,      "hincrbyfloat",     4,  4,         Write},
    {Hkeys,             "hkeys",            2,  2,         Read},
    {Hlen,              "hlen",             2,  2,         Read},
    {Hmget,             "hmget",            3,  MaxArgs,   Read},
    {Hmset,             "hmset",            4,  MaxArgs,   Write},
    {Hscan,             "hscan",            3,  7,         Read},
    {Hset,              "hset",             4,  MaxArgs,   Write},
    {Hsetnx,            "hsetnx",           4,  4,         Write},
    {Hstrlen,           "hstrlen",          3,  3,         Read},
    {Hvals,             "hvals",            2,  2,         Read},
    {Blpop,             "blpop",            3,  MaxArgs,   Write|Private},
    {Brpop,             "brpop",            3,  MaxArgs,   Write|Private},
    {Brpoplpush,        "brpoplpush",       4,  4,         Write|Private},
    {Lindex,            "lindex",           3,  3,         Read},
    {Linsert,           "linsert",          5,  5,         Write},
    {Llen,              "llen",             2,  2,         Read},
    {Lpop,              "lpop",             2,  2,         Write},
    {Lpush,             "lpush",            3,  MaxArgs,   Write},
    {Lpushx,            "lpushx",           3,  3,         Write},
    {Lrange,            "lrange",           4,  4,         Read},
    {Lrem,              "lrem",             4,  4,         Write},
    {Lset,              "lset",             4,  4,         Write},
    {Ltrim,             "ltrim",            4,  4,         Write},
    {Rpop,              "rpop",             2,  2,         Write},
    {Rpoplpush,         "rpoplpush",        3,  3,         Write},
    {Rpush,             "rpush",            3,  MaxArgs,   Write},
    {Rpushx,            "rpushx",           3,  3,         Write},
    {Sadd,              "sadd",             3,  MaxArgs,   Write},
    {Scard,             "scard",            2,  2,         Read},
    {Sdiff,             "sdiff",            2,  MaxArgs,   Read},
    {Sdiffstore,        "sdiffstore",       3,  MaxArgs,   Write},
    {Sinter,            "sinter",           2,  MaxArgs,   Read},
    {Sinterstore,       "sinterstore",      3,  MaxArgs,   Write},
    {Sismember,         "sismember",        3,  3,         Read},
    {Smembers,          "smembers",         2,  2,         Read},
    {Smove,             "smove",            4,  4,         Write},
    {Spop,              "spop",             2,  3,         Write},
    {Srandmember,       "srandmember",      2,  3,         Read},
    {Srem,              "srem",             3,  MaxArgs,   Write},
    {Sscan,             "sscan",            3,  7,         Read},
    {Sunion,            "sunion",           2,  MaxArgs,   Read},
    {Sunionstore,       "sunionstore",      3,  MaxArgs,   Write},
    {Zadd,              "zadd",             4,  MaxArgs,   Write},
    {Zcard,             "zcard",            2,  2,         Read},
    {Zcount,            "zcount",           4,  4,         Read},
    {Zincrby,           "zincrby",          4,  4,         Write},
    {Zinterstore,       "zinterstore",      4,  MaxArgs,   Write},
    {Zlexcount,         "zlexcount",        4,  4,         Read},
    {Zpopmax,           "zpopmax",          2,  3,         Write},
    {Zpopmin,           "zpopmin",          2,  3,         Write},
    {Zrange,            "zrange",           4,  5,         Read},
    {Zrangebylex,       "zrangebylex",      4,  7,         Read},
    {Zrangebyscore,     "zrangebyscore",    4,  8,         Read},
    {Zrank,             "zrank",            3,  3,         Read},
    {Zrem,              "zrem",             3,  MaxArgs,   Write},
    {Zremrangebylex,    "zremrangebylex",   4,  4,         Write},
    {Zremrangebyrank,   "zremrangebyrank",  4,  4,         Write},
    {Zremrangebyscore,  "zremrangebyscore", 4,  4,         Write},
    {Zrevrange,         "zrevrange",        4,  5,         Read},
    {Zrevrangebylex,    "zrevrangebylex",   4,  7,         Read},
    {Zrevrangebyscore,  "zrevrangebyscore", 4,  8,         Read},
    {Zrevrank,          "zrevrank",         3,  3,         Read},
    {Zscan,             "zscan",            3,  7,         Read},
    {Zscore,            "zscore",           3,  3,         Read},
    {Zunionstore,       "zunionstore",      4,  MaxArgs,   Write},
    {Pfadd,             "pfadd",            3,  MaxArgs,   Write},
    {Pfcount,           "pfcount",          2,  MaxArgs,   Read},
    {Pfmerge,           "pfmerge",          3,  MaxArgs,   Write},
    {Geoadd,            "geoadd",           5,  MaxArgs,   Write},
    {Geodist,           "geodist",          4,  5,         Read},
    {Geohash,           "geohash",          3,  MaxArgs,   Read},
    {Geopos,            "geopos",           3,  MaxArgs,   Read},
    {Georadius,         "georadius",        6,  16,        Write},
    {Georadiusbymember, "georadiusbymember",5,  15,        Write},
    {Psubscribe,        "psubscribe",       2,  MaxArgs,   Write|SMultiKey|Private},
    {Publish,           "publish",          3,  3,         Write},
    {Pubsub,            "pubsub",           2,  MaxArgs,   Read},
    {Punsubscribe,      "punsubscribe",     1,  MaxArgs,   Write|SMultiKey},
    {Subscribe,         "subscribe",        2,  MaxArgs,   Write|SMultiKey|Private},
    {Unsubscribe,       "unsubscribe",      1,  MaxArgs,   Write|SMultiKey},
    {SubMsg,            "\000SubMsg",       0,  0,         Admin}
};

int Command::Sentinel = Command::MaxCommands;
Command::CommandMap Command::CmdMap;

void Command::init()
{
    int type = 0;
    for (auto j = 0; j < MaxCommands; j++) {
        const auto& i = CmdPool[j];
        if (i.type != type) {
            Throw(InitFail, "command %s unmatch the index in commands table", i.name);
        }
        ++type;
        if (i.mode & (Command::Inner|Command::SubCmd)) {
            continue;
        }
        CmdMap[i.name] = &i;
    }
}

void Command::addCustomCommand(const CustomCommandConf& ccc) {
    if (Sentinel >= AvailableCommands) {
        Throw(InitFail, "too many custom commands(>%d)", MaxCustomCommands);
    }
    if (nullptr != find(ccc.name)) {
        Throw(InitFail, "custom command %s is duplicated", ccc.name.c_str()); 
    }
    auto* p = &CmdPool[Sentinel];
    p->name = ccc.name.c_str();
    p->minArgs = ccc.minArgs;
    p->maxArgs = ccc.maxArgs;
    p->mode = ccc.mode;
    p->type = (Command::Type)Sentinel++;
    CmdMap[ccc.name] = p;
}

