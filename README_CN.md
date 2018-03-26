# Predixy

**Predixy** 是一款高性能全特征redis代理，支持redis-sentinel和redis-cluster

## 特性

+ 高性能并轻量级
+ 支持多线程
+ 多平台支持：Linux、OSX、BSD、Windows([Cygwin](http://www.cygwin.com/))
+ 支持Redis Sentinel，可配置一组或者多组redis
+ 支持Redis Cluster
+ 支持redis阻塞型命令，包括blpop、brpop、brpoplpush
+ 支持scan命令，无论是单个redis还是多个redis实例都支持
+ 多key命令支持: mset/msetnx/mget/del/unlink/touch/exists
+ 支持redis的多数据库，即可以使用select命令
+ 支持事务，当前仅限于Redis Sentinel下单一redis组可用
+ 支持脚本，包括命令：script load、eval、evalsha
+ 支持发布订阅机制，也即Pub/Sub系列命令
+ 多数据中心支持，读写分离支持
+ 扩展的AUTH命令，强大的读、写、管理权限控制机制，健空间限制机制
+ 日志可按级别采样输出，异步日志记录避免线程被io阻塞
+ 日志文件可以按时间、大小自动切分
+ 丰富的统计信息，包括CPU、内存、请求、响应等信息
+ 延迟监控信息，可以看到整体延迟，分后端redis实例延迟

## 编译

Predixy可以在所有主流平台下编译，推荐在linux下使用，需要支持C++11的编译器。

编译非常简单，下载或者git clone代码后进到predixy目录，直接执行：

    $ make

编译后会在src目录生成一个可执行文件predixy

编译debug版本:

    $ make debug

更多编译选项：

+ CXX=c++compiler，指定c++编译器，缺省是g++，可以指定为其它，例如：CXX=clang++
+ EV=epoll|poll|kqueue，指定异步io模型，缺省情况下是根据平台来检测
+ MT=false, 关闭多线程支持
+ TS=true, 开启函数调用耗时分析，该选项仅用于开发模式

一些使用参数编译的例子：

    $ make CXX=clang++
    $ make EV=poll
    $ make MT=false
    $ make debug MT=false TS=true

## 安装

简单的只要拷贝src/predixy到目标路径即可

    $ cp src/predixy /path/to/bin

## 配置 [详细文档](https://github.com/joyieldInc/predixy/blob/master/doc/config_CN.md)

predixy的配置类似redis, 具体配置项的含义在配置文件里有详细解释，请参考下列配置文件：

+ predixy.conf，整体配置文件，会引用下面的配置文件
+ cluster.conf，用于Redis Cluster时，配置后端redis信息
+ sentinel.conf，用于Redis Sentinel时，配置后端redis信息
+ auth.conf，访问权限控制配置，可以定义多个验证密码，可每个密码指定读、写、管理权限，以及定义可访问的健空间
+ dc.conf，多数据中心支持，可以定义读写分离规则，读流量权重分配
+ latency.conf， 延迟监控规则定义，可以指定需要监控的命令以及延时时间间隔

提供这么多配置文件实际上是按功能分开了，所有配置都可以写到一个文件里，也可以写到多个文件里然后在主配置文件里引用进来。

## 运行

    $ src/predixy conf/predixy.conf

使用默认的配置文件predixy.conf， predixy将监听地址0.0.0.0:7617，后端的redis是Redis Cluster 127.0.0.1:6379。通常，127.0.0.1:6379并不是运行在Redis Clusterr模式下，因此Predixy将会有大量的错误日志输出。不过你依然可以用redis-cli连接predixy来试用一下：

    $ redis-cli -p 7617 info

执行上条命令后可以看到predixy自身的一些信息，如果127.0.0.1:6379在运行的话，你可以试试其它redis命令，看看效果如何。

更多的启动命令行参数请看帮助：

    $ src/predixy -h

## 统计信息

和redis一样，predixy用INFO命令来给出统计信息。

在redis-cli连接下执行下面的命令：

    redis> INFO

你将看到类似redis执行INFO命令的输出，不过这里是predixy的统计信息。

特别提一下predixy里面的延迟监控信息，可以通过在配置里定义的延迟监控名来看延迟信息

    redis> INFO Latency <latency-name>

下面是一个延迟信息输出的例子：

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
    The last line is total summary, 60 is average latency(us)

还可以单独看某个redis后端的延迟信息：

    redis> INFO ServerLatency <server-address> [latency-name]

要重置所有的统计信息，和redis执行的命令是一样的，不过predixy要求有管理权限才可以重置统计信息

    redis> CONFIG ResetStat

## 性能评测

predixy很快，有多快？对比几个流行的redis代理(twemproxy,codis,redis-cerberus), predixy要比它们快得多。

具体比较参见Wiki
[benchmark](https://github.com/joyieldInc/predixy/wiki/Benchmark)

## 许可

Copyright (C) 2017 Joyield, Inc. <joyield.com#gmail.com>

All rights reserved.

License under BSD 3-clause "New" or "Revised" License

微信:cppfan ![微信](https://github.com/joyieldInc/predixy/blob/master/doc/wechat-cppfan.jpeg)
