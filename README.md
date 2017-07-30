# Predixy 

**Predixy** is a high performance and full features proxy for [redis](http://redis.io/)

## Features

+ High performance and lightweight.
+ Multi-threads support.
+ Works on Linux, OSX, BSD, Windows([Cygwin](http://www.cygwin.com/)).
+ Supports Redis Sentinel, single/multi redis group[s].
+ Supports Redis Cluster.
+ Supports redis block command, eg:blpop, brpop, brpoplpush.
+ Supports scan command, even multi redis instances.
+ Multi-databases support, means redis command select is avaliable.
+ Supports redis transaction, limit in Redis Sentinel single redis group.
+ Supports redis Scripts, script load, eval, evalsha.
+ Supports redis Pub/Sub.
+ Multi-DataCenters support.
+ Extend AUTH, readonly/readwrite/admin permission, keyspace limit.
+ Log level sample, async log record.
+ Log file auto rotate by time and/or file size.
+ Stats info, CPU/Memory/Requests/Responses and so on.
+ Latency monitor.

## Build

Predixy can be compiled and used on Linux, OSX, BSD, Windows([Cygwin](http://www.cygwin.com/)).

It is as simple as:

    $ make

To build in debug mode:

    $ make debug

Some other build options:
+ CXX=c++compiler, default is g++, you can specify other, eg:CXX=clang++
+ EV=epoll|poll|kqueue, default it is auto detect according by platform.
+ MT=false, disable multi-threads support.
+ TS=true, enable predixy function call time stats, debug only for developer.

For examples:

    $ make CXX=clang++
    $ make EV=poll
    $ make MT=false
    $ make debug MT=false TS=true

## Configuration

See below files:
+ predixy.conf, basic config.
+ cluster.conf, Redis Cluster backend config.
+ sentinel.conf, Redis Sentinel backend config.
+ auth.conf, authority control config.
+ dc.conf, multi-datacenters config.
+ latency.conf, latency monitor config.

## Running

    $ ./predixy ../conf/predixy.conf

With default predixy.conf, Predixy will proxy to Redis Cluster 127.0.0.1:6379,
In general, 127.0.0.1:6379 is not running in Redis Cluster mode,
So you will look mass log output. But you can still test it with redis-cli.

    $ redis-cli -p 7617 info

More command line arguments:

    $ ./predixy -h

## Stats

Like redis, predixy use INFO command to give stats.

Show predixy running info and latency monitors

    redis> INFO

Show latency monitors by latency name

    redis> INFO Latency <latency-name>

A latency monitor example:

    LatencyMonitorName:all
                latency(us)   sum(us)           counts
    <=          100              3769836            91339 91.34%
    <=          200               777185             5900 97.24%
    <=          300               287565             1181 98.42%
    <=          400               185891              537 98.96%
    <=          500               132773              299 99.26%
    <=          600                85050              156 99.41%
    <=          700                85455              133 99.54%
    <=          800                40088               54 99.60%
    <=         1000                67788               77 99.68%
    >          1000               601012              325 100.00%
    T            60              6032643           100001
    The last line is total summary, 624 is average latency(us)


Show latency monitors by server address and latency name

    redis> INFO ServerLatency <server-address> [latency-name]

Reset all stats and latency monitors

    redis> CONFIG ResetStat

## License

Copyright (C) 2017 Joyield, Inc. <joyield.com#gmail.com>

All rights reserved.

License under BSD 3-clause "New" or "Revised" License
