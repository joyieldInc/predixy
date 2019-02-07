#!/usr/bin/env python
# 
#  predixy - A high performance and full features proxy for redis.
#  Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
#  All rights reserved.
#

import time
import redis
import sys
import argparse

c = None

Cases = [
    ('ping', [
        [('ping',), 'PONG'],
    ]),
    ('echo', [
        [('echo', 'hello'), 'hello'],
    ]),
    ('del', [
        [('set', 'key', 'val'), 'OK'],
        [('del', 'key'), 1],
        [('del', 'key'), 0],
        [('mset', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'), 'OK'],
        [('del', 'a', 'b', 'c'), 2],
        [('del', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'), 2],
    ]),
    ('dump', [
        [('set', 'k', 'v'), 'OK'],
        [('dump', 'k'), lambda x:len(x)>10],
        [('del', 'k'), 1],
    ]),
    ('exists', [
        [('set', 'k', 'v'), 'OK'],
        [('exists', 'k'), 1],
        [('del', 'k'), 1],
        [('exists', 'k'), 0],
        [('mset', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'), 'OK'],
        [('exists', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'), 4],
        [('del', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'), 4],
    ]),
    ('rename', [
        [('del', '{k}1', '{k}2'), ],
        [('set', '{k}1', 'v'), 'OK'],
        [('rename', '{k}1', '{k}2'), 'OK'],
        [('get', '{k}2'), 'v'],
    ]),
    ('renamenx', [
        [('del', '{k}1', '{k}2'), ],
        [('set', '{k}1', 'v'), 'OK'],
        [('renamenx', '{k}1', '{k}2'), 1],
        [('get', '{k}2'), 'v'],
        [('set', '{k}1', 'new'), 'OK'],
        [('renamenx', '{k}1', '{k}2'), 0],
    ]),
    ('expire', [
        [('set', 'k', 'v'), 'OK'],
        [('ttl', 'k'), -1],
        [('expire', 'k', 10), 1],
        [('ttl', 'k'), lambda x: x>0],
        [('del', 'k'), 1],
    ]),
    ('pexpire', [
        [('set', 'k', 'v'), 'OK'],
        [('ttl', 'k'), -1],
        [('pexpire', 'k', 10000), 1],
        [('ttl', 'k'), lambda x: x>0],
        [('del', 'k'), 1],
    ]),
    ('expireat', [
        [('set', 'k', 'v'), 'OK'],
        [('ttl', 'k'), -1],
        [('expireat', 'k', int(time.time()) + 10), 1],
        [('ttl', 'k'), lambda x: x>0],
        [('del', 'k'), 1],
    ]),
    ('pexpireat', [
        [('set', 'k', 'v'), 'OK'],
        [('pttl', 'k'), -1],
        [('pexpireat', 'k', (int(time.time()) + 10) * 1000) , 1],
        [('pttl', 'k'), lambda x: x>0],
        [('del', 'k'), 1],
    ]),
    ('persist', [
        [('setex', 'k', 10, 'v'), 'OK'],
        [('ttl', 'k'), lambda x: x>0],
        [('persist', 'k'), 1],
        [('ttl', 'k'), -1],
        [('del', 'k'), 1],
    ]),
    ('pttl', [
        [('set', 'k', 'v'), 'OK'],
        [('pttl', 'k'), -1],
        [('setex', 'k', 10, 'v'), 'OK'],
        [('pttl', 'k'), lambda x: x>0],
        [('del', 'k'), 1],
    ]),
    ('ttl', [
        [('set', 'k', 'v'), 'OK'],
        [('ttl', 'k'), -1],
        [('setex', 'k', 10, 'v'), 'OK'],
        [('ttl', 'k'), lambda x: x>0],
        [('del', 'k'), 1],
    ]),
    ('type', [
        [('set', 'k', 'v'), 'OK'],
        [('del', 'h', 'l', 's', 'z'),],
        [('type', 'k'), 'string'],
        [('hset', 'h', 'k', 'v'), 1],
        [('type', 'h'), 'hash'],
        [('lpush', 'l', 'k'), 1],
        [('type', 'l'), 'list'],
        [('sadd', 's', 'k'), 1],
        [('type', 's'), 'set'],
        [('zadd', 'z', 10, 'k'), 1],
        [('type', 'z'), 'zset'],
        [('del', 'k', 'h', 'l', 's', 'z'), 5],
    ]),
    ('sort', [
        [('del', 'list'), ],
        [('lpush', 'list', 6, 3, 1, 2, 5, 4), 6],
        [('sort', 'list'), ['1', '2', '3', '4', '5', '6']],
        [('sort', 'list', 'ASC'), ['1', '2', '3', '4', '5', '6']],
        [('sort', 'list', 'DESC'), ['6', '5', '4', '3', '2', '1']],
        [('sort', 'list', 'LIMIT', 1, 2), ['2', '3']],
        [('mset', 'u1', -1, 'u2', -2, 'u3', -3, 'u4', -4, 'u5', -5, 'u6', -6), 'OK'],
        [('del', 'list'), 1],
        [('lpush', 'list', 'c++', 'java', 'c', 'javascript', 'python'), 5],
        [('sort', 'list', 'ALPHA'), ['c', 'c++', 'java', 'javascript', 'python']],
        [('sort', 'list', 'DESC', 'ALPHA'), ['python', 'javascript', 'java', 'c++', 'c']],
        [('del', 'list', 'u1', 'u2', 'u3', 'u4', 'u5', 'u6'), 7],
    ]),
    ('touch', [
        [('del', 'k1', 'k2', 'k3', 'k4'), ],
        [('mset', 'k1', 'v1', 'k2', 'v2', 'k3', 'v3'), 'OK'],
        [('touch', 'k1'), 1],
        [('touch', 'k1', 'k2', 'k3', 'k4'), 3],
        [('del', 'k1', 'k2', 'k3'), 3],
        [('touch', 'k1', 'k2', 'k3', 'k4'), 0],
    ]),
    ('scan', [
        [('mset', 'k1', 'v1', 'k2', 'v2', 'k3', 'v3'), 'OK'],
        [('scan', '0'), lambda x: x[0] != 0],
        [('scan', '0', 'count', 1), lambda x: x[0] != 0],
        [('del', 'k1', 'k2', 'k3'), 3],
    ]),
    ('append', [
        [('set', 'k', ''), 'OK'],
        [('strlen', 'k'), 0],
        [('append', 'k', '1'), 1],
        [('get', 'k'), '1'],
        [('append', 'k', '2'), 2],
        [('get', 'k'), '12'],
    ]),
    ('bitcount', [
        [('set', 'k', '\x0f\x07\x03\x01'), 'OK'],
        [('bitcount', 'k'), 10],
        [('bitcount', 'k', 0, 1), 7],
        [('bitcount', 'k', -2, -1), 3],
    ]),
    ('bitfield', [
        [('set', 'k', 123), 'OK'],
        [('bitfield', 'k', 'INCRBY', 'i5', 2, 3), [-5]],
    ]),
    ('bitop', [
        [('del', '{k}1', '{k}2', '{k}3'), ],
        [('set', '{k}1', '\x0f'), 'OK'],
        [('set', '{k}2', '\xf1'), 'OK'],
        [('bitop', 'NOT', '{k}3', '{k}1'), 1],
        [('bitop', 'AND', '{k}3', '{k}1', '{k}2'), 1],
    ]),
    ('bitpos', [
        [('set', 'k', '\x0f'), 'OK'],
        [('bitpos', 'k', 0), 0],
        [('bitpos', 'k', 1, 0, -1), 4],
    ]),
    ('decr', [
        [('set', 'k', 10), 'OK'],
        [('decr', 'k'), 9],
        [('decr', 'k'), 8],
    ]),
    ('decrby', [
        [('set', 'k', 10), 'OK'],
        [('decrby', 'k', 2), 8],
        [('decrby', 'k', 3), 5],
    ]),
    ('getbit', [
        [('set', 'k', '\x0f'), 'OK'],
        [('getbit', 'k', 0), 0],
        [('getbit', 'k', 4), 1],
    ]),
    ('getrange', [
        [('set', 'k', '0123456'), 'OK'],
        [('getrange', 'k', 0, 2), '012'],
        [('getrange', 'k', -2, -1), '56'],
    ]),
    ('getset', [
        [('set', 'k', 'v'), 'OK'],
        [('getset', 'k', 'value'), 'v'],
        [('get', 'k'), 'value'],
    ]),
    ('incr', [
        [('set', 'k', 10), 'OK'],
        [('incr', 'k'), 11],
        [('incr', 'k'), 12],
    ]),
    ('incrby', [
        [('set', 'k', 10), 'OK'],
        [('incrby', 'k', 2), 12],
        [('incrby', 'k', 3), 15],
    ]),
    ('incrbyfloat', [
        [('set', 'k', 10), 'OK'],
        [('incrbyfloat', 'k', 2.5), '12.5'],
        [('incrbyfloat', 'k', 3.5), '16'],
    ]),
    ('mget', [
        [('mset', 'k', 'v'), 'OK'],
        [('mget', 'k'), ['v']],
        [('mget', 'k', 'k'), ['v', 'v']],
        [('mset', 'k1', 'v1', 'k2', 'v2'), 'OK'],
        [('mget', 'k1', 'v1', 'k2', 'v2'), ['v1', None, 'v2', None]],
        [('del', 'k1', 'k2'), 2],
    ]),
    ('msetnx', [
        [('del', 'k1', 'k2', 'k3'), ],
        [('msetnx', 'k1', 'v1', 'k2', 'v2', 'k3', 'v3'), 1],
        [('mget', 'k1', 'k2', 'k3'), ['v1', 'v2', 'v3']],
        [('msetnx', 'k1', 'v1', 'k2', 'v2', 'k3', 'v3'), 0],
        [('del', 'k1', 'k2', 'k3'), 3],
    ]),
    ('psetex', [
        [('del', 'k'), ],
        [('psetex', 'k', 10000, 'v'), 'OK'],
        [('get', 'k'), 'v'],
        [('pttl', 'k'), lambda x: x>0],
    ]),
    ('set', [
        [('set', 'k', 'v'), 'OK'],
        [('get', 'k'), 'v'],
        [('ttl', 'k'), -1],
        [('set', 'k', 'vex', 'EX', 10), 'OK'],
        [('get', 'k'), 'vex'],
        [('ttl', 'k'), lambda x: x>0],
        [('set', 'k', 'vpx', 'PX', 20000), 'OK'],
        [('get', 'k'), 'vpx'],
        [('pttl', 'k'), lambda x: x>10000],
        [('set', 'k', 'val', 'NX'), None],
        [('get', 'k'), 'vpx'],
        [('set', 'k', 'val', 'XX'), 'OK'],
        [('get', 'k'), 'val'],
    ]),
    ('setbit', [
        [('set', 'k', '\x00'), 'OK'],
        [('setbit', 'k', 1, 1), 0],
        [('setbit', 'k', 1, 0), 1],
    ]),
    ('setex', [
        [('del', 'k'), ],
        [('setex', 'k', 10, 'v'), 'OK'],
        [('get', 'k'), 'v'],
        [('ttl', 'k'), lambda x: x>0],
    ]),
    ('setnx', [
        [('del', 'k'), ],
        [('setnx', 'k', 'v'), 1],
        [('get', 'k'), 'v'],
        [('setnx', 'k', 'v'), 0],
    ]),
    ('setrange', [
        [('set', 'k', 'hello world'), 'OK'],
        [('setrange', 'k', 6, 'predixy'), 13],
    ]),
    ('strlen', [
        [('set', 'k', '123456'), 'OK'],
        [('strlen', 'k'), 6],
    ]),
    ('script', [
        [('del', 'k'), ],
        [('eval', 'return "hello"', 0), 'hello'],
        [('eval', 'return KEYS[1]', 1, 'k'), 'k'],
        [('eval', 'return KEYS[1]', 3, '{k}1', '{k}2', '{k}3'), '{k}1'],
        [('eval', 'return redis.call("set", KEYS[1], ARGV[1])', 1, 'k', 'v'), 'OK'],
        [('eval', 'return redis.call("get", KEYS[1])', 1, 'k'), 'v'],
        [('script', 'load', 'return redis.call("get", KEYS[1])'), 'a5260dd66ce02462c5b5231c727b3f7772c0bcc5'],
        [('evalsha', 'a5260dd66ce02462c5b5231c727b3f7772c0bcc5', 1, 'k'), 'v'],
    ]),
    ('hash', [
        [('del', 'k'), ],
        [('hset', 'k', 'name', 'hash'), 1],
        [('hget', 'k', 'name'), 'hash'],
        [('hexists', 'k', 'name'), 1],
        [('hlen', 'k'), 1],
        [('hkeys', 'k'), ['name']],
        [('hgetall', 'k'), ['name', 'hash']],
        [('hmget', 'k', 'name'), ['hash']],
        [('hscan', 'k', 0), ['0', ['name', 'hash']]],
        [('hstrlen', 'k', 'name'), 4],
        [('hvals', 'k'), ['hash']],
        [('hsetnx', 'k', 'name', 'other'), 0],
        [('hget', 'k', 'name'), 'hash'],
        [('hsetnx', 'k', 'age', 5), 1],
        [('hget', 'k', 'age'), '5'],
        [('hincrby', 'k', 'age', 3), 8],
        [('hincrbyfloat', 'k', 'age', 1.5), '9.5'],
        [('hmset', 'k', 'sex', 'F'), 'OK'],
        [('hget', 'k', 'sex'), 'F'],
        [('hmset', 'k', 'height', 180, 'weight', 80, 'zone', 'cn'), 'OK'],
        [('hlen', 'k'), 6],
        [('hmget', 'k', 'name', 'age', 'sex', 'height', 'weight', 'zone'), ['hash', '9.5', 'F', '180', '80', 'cn']],
        [('hscan', 'k', 0, 'match', '*eight'), lambda x:False if len(x)!=2 else len(x[1])==4],
        [('hscan', 'k', 0, 'count', 2), lambda x:len(x)==2],
        [('hkeys', 'k'), lambda x:len(x)==6],
        [('hvals', 'k'), lambda x:len(x)==6],
        [('hgetall', 'k'), lambda x:len(x)==12],
    ]),
    ('list', [
        [('del', 'k'), ],
        [('lpush', 'k', 'apple'), 1],
        [('llen', 'k'), 1],
        [('lindex', 'k', 0), 'apple'],
        [('lindex', 'k', -1), 'apple'],
        [('lindex', 'k', -2), None],
        [('lpush', 'k', 'pear', 'orange'), 3],
        [('llen', 'k'), 3],
        [('lrange', 'k', 0, 3), ['orange', 'pear', 'apple']],
        [('lrange', 'k', -2, -1), ['pear', 'apple']],
        [('lset', 'k', 0, 'peach'), 'OK'],
        [('lindex', 'k', 0), 'peach'],
        [('rpush', 'k', 'orange'), 4],
        [('lrange', 'k', 0, 3), ['peach', 'pear', 'apple', 'orange']],
        [('rpush', 'k', 'grape', 'banana', 'tomato'), 7],
        [('lrange', 'k', 0, 7), ['peach', 'pear', 'apple', 'orange', 'grape', 'banana', 'tomato']],
        [('lpop', 'k'), 'peach'],
        [('rpop', 'k'), 'tomato'],
        [('rpoplpush', 'k', 'k'), 'banana'],
        [('lpushx', 'k', 'peach'), 6],
        [('rpushx', 'k', 'peach'), 7],
        [('lrem', 'k', 1, 'apple'), 1],
        [('lrem', 'k', 5, 'peach'), 2],
        [('lrange', 'k', 0, 7), ['banana', 'pear', 'orange', 'grape']],
        [('linsert', 'k', 'BEFORE', 'pear', 'peach'), 5],
        [('linsert', 'k', 'AFTER', 'orange', 'tomato'), 6],
        [('linsert', 'k', 'AFTER', 'apple', 'tomato'), -1],
        [('lrange', 'k', 0, 7), ['banana', 'peach', 'pear', 'orange', 'tomato', 'grape']],
        [('ltrim', 'k', 0, 4), 'OK'],
        [('ltrim', 'k', 1, -1), 'OK'],
        [('lrange', 'k', 0, 7), ['peach', 'pear', 'orange', 'tomato']],
        [('blpop', 'k', 0), ['k', 'peach']],
        [('brpop', 'k', 0), ['k', 'tomato']],
        [('brpoplpush', 'k', 'k', 0), 'orange'],
        [('lrange', 'k', 0, 7), ['orange', 'pear']],
        [('del', 'k'), 1],
        [('lpushx', 'k', 'peach'), 0],
        [('rpushx', 'k', 'peach'), 0],
    ]),
    ('set', [
        [('del', 'k', '{k}2', '{k}3', '{k}4', '{k}5', '{k}6'), ],
        [('sadd', 'k', 'apple'), 1],
        [('scard', 'k'), 1],
        [('sadd', 'k', 'apple'), 0],
        [('scard', 'k'), 1],
        [('sadd', 'k', 'apple', 'pear', 'orange', 'banana'), 3],
        [('scard', 'k'), 4],
        [('sismember', 'k', 'apple'), 1],
        [('sismember', 'k', 'grape'), 0],
        [('smembers', 'k'), lambda x:len(x)==4],
        [('srandmember', 'k'), lambda x:x in ['apple', 'pear', 'orange', 'banana']],
        [('srandmember', 'k', 2), lambda x:len(x)==2],
        [('sscan', 'k', 0), lambda x:len(x)==2],
        [('sscan', 'k', 0, 'match', 'a*'), lambda x:len(x)==2 and x[1][0]=='apple'],
        [('sscan', 'k', 0, 'count', 2), lambda x:len(x)==2 and len(x[1])>=2],
        [('srem', 'k', 'apple'), 1],
        [('srem', 'k', 'apple'), 0],
        [('scard', 'k'), 3],
        [('srem', 'k', 'pear', 'orange'), 2],
        [('scard', 'k'), 1],
        [('sadd', '{k}2', 'apple', 'pear', 'orange', 'banana'), 4],
        [('sdiff', '{k}2', 'k'), lambda x:len(x)==3],
        [('sadd', '{k}3', 'apple', 'pear'), 2],
        [('sdiff', '{k}2', 'k', '{k}3'), ['orange']],
        [('sdiffstore', '{k}4', '{k}2', 'k', '{k}3'), 1],
        [('sinter', '{k}2', 'k'), ['banana']],
        [('sinterstore', '{k}5', '{k}2', 'k'), 1],
        [('sunion', '{k}3', 'k'), lambda x:len(x)==3],
        [('sunionstore', '{k}6', '{k}3', 'k'), 3],
        [('smove', '{k}2', 'k', 'apple'), 1],
        [('scard', 'k'), 2],
        [('scard', '{k}2'), 3],
    ]),
    ('zset', [
        [('del', 'k', '{k}2', '{k}3', '{k}4', '{k}5', '{k}6'), ],
        [('zadd', 'k', 10, 'apple'), 1],
        [('zcard', 'k'), 1],
        [('zincrby', 'k', 2, 'apple'), '12'],
        [('zincrby', 'k', -2, 'apple'), '10'],
        [('zadd', 'k', 15, 'pear', 20, 'orange', 30, 'banana'), 3],
        [('zcard', 'k'), 4],
        [('zscore', 'k', 'pear'), '15'],
        [('zrank', 'k', 'apple'), 0],
        [('zrank', 'k', 'orange'), 2],
        [('zcount', 'k', '-inf', '+inf'), 4],
        [('zcount', 'k', 1, 10), 1],
        [('zcount', 'k', 15, 20), 2],
        [('zlexcount', 'k', '[a', '[z'), 4],
        [('zscan', 'k', 0), lambda x:len(x)==2 and len(x[1])==8],
        [('zscan', 'k', 0, 'MATCH', 'o*'), ['0', ['orange', '20']]],
        [('zrange', 'k', 0, 2), ['apple', 'pear', 'orange']],
        [('zrange', 'k', -2, -1), ['orange', 'banana']],
        [('zrange', 'k', 0, 2, 'WITHSCORES'), ['apple', '10', 'pear', '15', 'orange', '20']],
        [('zrangebylex', 'k', '-', '+'), lambda x:len(x)==4],
        [('zrangebylex', 'k', '-', '+', 'LIMIT', 1, 2), lambda x:len(x)==2],
        [('zrangebyscore', 'k', '10', '(20'), ['apple', 'pear']],
        [('zrangebyscore', 'k', '-inf', '+inf', 'LIMIT', 1, 2), ['pear', 'orange']],
        [('zrangebyscore', 'k', '-inf', '+inf', 'WITHSCORES', 'LIMIT', 1, 2), ['pear', '15', 'orange', '20']],
        [('zrevrange', 'k', 0, 2), ['banana', 'orange', 'pear']],
        [('zrevrange', 'k', -2, -1), ['pear', 'apple']],
        [('zrevrange', 'k', 0, 2, 'WITHSCORES'), ['banana', '30', 'orange', '20', 'pear', '15']],
        [('zrevrangebylex', 'k', '+', '-'), lambda x:len(x)==4],
        [('zrevrangebylex', 'k', '+', '-', 'LIMIT', 1, 2), lambda x:len(x)==2],
        [('zrevrangebyscore', 'k', '(20', '10'), ['pear', 'apple']],
        [('zrevrangebyscore', 'k', '+inf', '-inf', 'LIMIT', 1, 2), ['orange', 'pear']],
        [('zrevrangebyscore', 'k', '+inf', '-inf', 'WITHSCORES', 'LIMIT', 1, 2), ['orange', '20', 'pear', '15']],
        [('zrem', 'k', 'apple'), 1],
        [('zrem', 'k', 'apple'), 0],
        [('zremrangebyrank', 'k', '0', '1'), 2],
        [('zadd', 'k', 15, 'pear', 20, 'orange', 30, 'banana'), 2],
        [('zremrangebyscore', 'k', '20', '30'), 2],
        [('zadd', 'k', 'NX', 0, 'pear', 0, 'orange', 0, 'banana'), 2],
        [('zremrangebylex', 'k', '[banana', '(cat'), 1],
        [('zadd', 'k', 15, 'pear', 20, 'orange', 30, 'banana'), 1],
        [('zadd', '{k}2', 10, 'apple', 15, 'pear'), 2],
        [('zinterstore', '{k}3', 2, 'k', '{k}2'), 1],
        [('zinterstore', '{k}3', 2, 'k', '{k}2', 'AGGREGATE', 'MAX'), 1],
        [('zinterstore', '{k}3', 2, 'k', '{k}2', 'WEIGHTS', 0.5, 1.2, 'AGGREGATE', 'MAX'), 1],
        [('zunionstore', '{k}3', 2, 'k', '{k}2'), 4],
        [('zunionstore', '{k}3', 2, 'k', '{k}2', 'AGGREGATE', 'MAX'), 4],
        [('zunionstore', '{k}3', 2, 'k', '{k}2', 'WEIGHTS', 0.5, 1.2, 'AGGREGATE', 'MAX'), 4],
        [('zadd', '{k}5', 0, 'apple', 9, 'banana', 1, 'pear', 3, 'orange', 4, 'cat'), 5],
        [('zpopmax', '{k}5'), ['banana', '9']],
        [('zpopmax', '{k}5', 3), ['cat', '4', 'orange', '3', 'pear', '1']],
        [('zadd', '{k}6', 0, 'apple', 9, 'banana', 1, 'pear', 3, 'orange', 4, 'cat'), 5],
        [('zpopmin', '{k}6'), ['apple', '0']],
        [('zpopmin', '{k}6', 3), ['pear', '1', 'orange', '3', 'cat', '4']],
    ]),
    ('hyperloglog', [
        [('del', 'k', '{k}2', '{k}3'), ],
        [('pfadd', 'k', 'a', 'b', 'c', 'd'), 1],
        [('pfcount', 'k'), 4],
        [('pfadd', '{k}2', 'c', 'd', 'e', 'f'), 1],
        [('pfcount', '{k}2'), 4],
        [('pfmerge', '{k}3', 'k', '{k}2'), 'OK'],
        [('pfcount', '{k}3'), 6],
    ]),
    ('geo', [
        [('del', 'k'), ],
        [('geoadd', 'k', 116, 40, 'beijing'), 1],
        [('geoadd', 'k', 121.5, 30.8, 'shanghai', 114, 22.3, 'shenzhen'), 2],
        [('geoadd', 'k', -74, 40.3, 'new york', 151.2, -33.9, 'sydney'), 2],
        [('geodist', 'k', 'beijing', 'shanghai'), lambda x:x>1000000],
        [('geodist', 'k', 'beijing', 'shanghai', 'km'), lambda x:x>1000],
        [('geohash', 'k', 'beijing', 'shanghai'), lambda x:len(x)==2],
        [('geopos', 'k', 'beijing'), lambda x:len(x)==1 and len(x[0])==2],
        [('geopos', 'k', 'beijing', 'shanghai'), lambda x:len(x)==2 and len(x[1])==2],
        [('georadius', 'k', 140, 35, 3000, 'km'), lambda x:len(x)==3],
        [('georadius', 'k', 140, 35, 3000, 'km', 'WITHDIST', 'ASC'), lambda x:len(x)==3 and x[0][0]=='shanghai' and x[1][0]=='beijing' and x[2][0]=='shenzhen'],
        [('georadiusbymember', 'k', 'shanghai', 2000, 'km'), lambda x:len(x)==3],
        [('georadiusbymember', 'k', 'shanghai', 3000, 'km', 'WITHDIST', 'ASC'), lambda x:len(x)==3 and x[0][0]=='shanghai' and x[1][0]=='beijing' and x[2][0]=='shenzhen'],
    ]),
    ('clean', [
        [('del', 'k'), ],
    ]),
]

TransactionCases = [
    ('multi-exec', [
        [('multi',), 'OK'],
        [('set', 'k', 'v'), 'QUEUED'],
        [('get', 'k'), 'QUEUED'],
        [('exec',), ['OK', 'v']],
    ]),
    ('multi-discard', [
        [('multi',), 'OK'],
        [('set', 'k', 'v'), 'QUEUED'],
        [('get', 'k'), 'QUEUED'],
        [('discard',), 'OK'],
    ]),
    ('watch-multi-exec', [
        [('watch', 'k'), 'OK'],
        [('watch', '{k}2', '{k}3'), 'OK'],
        [('multi',), 'OK'],
        [('set', 'k', 'v'), 'QUEUED'],
        [('get', 'k'), 'QUEUED'],
        [('exec',), ['OK', 'v']],
    ]),
]

def check(cmd, r):
    if len(cmd) == 1:
        print('EXEC %s' % (str(cmd[0]),))
        return True
    if hasattr(cmd[1], '__call__'):
        isPass = cmd[1](r)
    else:
        isPass = r == cmd[1]
    if isPass:
        print('PASS %s:%s' % (str(cmd[0]), repr(r)))
    else:
        print('FAIL %s:%s != %s' % (str(cmd[0]), repr(r), repr(cmd[1])))
        return False
    return True


def testCase(name, cmds):
    print('----------  %s  --------' % name)
    succ = True
    for cmd in cmds:
        try:
            r = c.execute_command(*cmd[0])
            if not check(cmd, r):
                succ = False
        except Exception as excp:
            succ = False
            if len(cmd) > 1:
                print('EXCP %s:%s %s' % (str(cmd[0]), str(cmd[1]), str(excp)))
            else:
                print('EXCP %s %s' % (str(cmd[0]), str(excp)))
    return succ

def pipelineTestCase(name, cmds):
    print('----------  %s pipeline --------' % name)
    succ = True
    p = c.pipeline(transaction=False)
    try:
        for cmd in cmds:
            p.execute_command(*cmd[0])
        res = p.execute()
        for i in xrange(0, len(cmds)):
            if not check(cmds[i], res[i]):
                succ = False
    except Exception as excp:
        succ = False
        print('EXCP %s' % str(excp))
    return succ

if __name__ == '__main__':
    parser = argparse.ArgumentParser(conflict_handler='resolve')
    parser.add_argument('-t', default=False, action='store_true', help='enable transaction test')
    parser.add_argument('-h', nargs='?', default='127.0.0.1', help='host')
    parser.add_argument('-p', nargs='?', default=7617, type=int, help='port')
    parser.add_argument('case', nargs='*', default=None, help='specify test case')
    args = parser.parse_args()
    a = set()
    host = '127.0.0.1' if not args.h else args.h
    port = 7617 if not args.p else args.p
    c = redis.StrictRedis(host=host, port=port)
    if args.case:
        a = set(args.case)
    fails = []
    for case in Cases:
        if len(a) == 0 or case[0] in a:
            if not testCase(case[0], case[1]) or not pipelineTestCase(case[0], case[1]):
                fails.append(case[0])
    if args.t or 'transaction' in a:
        succ = True
        for case in TransactionCases:
            if not pipelineTestCase(case[0], case[1]):
                succ = False
        if not succ:
            fails.append('transaction')
    print('--------------------------------------------')
    if len(fails) > 0:
        print('******* Some case test fail *****')
        for cmd in fails:
            print cmd
    else:
        print('Good! all Case Pass.')

