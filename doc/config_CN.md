# Predixy配置文档说明

要正常运行predixy服务，一个配置文件是必不可少的，启动一个正常的predixy服务需执行以下命令：

    $ predixy <config-file> [--ArgName=ArgValue]...

predixy首先读取config-file文件中的配置信息，如果有命令行参数指定的话，则会用命令行参数的值覆盖配置文件中定义的相应值。

## 配置文件格式说明
配置文件是以行为单位的文本文件，每一行是以下几种类型之一

+ 空行或注释
+ Key Value 
+ Key Value {
+ }

### 规则

+ 以#开始的内容为注释内容
+ Include是一个特殊的Key，表示引用Value指明的文件，如果不是绝对路径的话，则相对路径是当前这个文件所在的路径
+ Value可以为空， 如果Value本身包括#，双引号的话，应该用双引号括起来，里面的双引号加反斜杠转义, 例如: "A \"#special#\" value"
+ 多个行定义同一个Key的话，最后出现的行会覆盖之前的定义


## 基本配置部分

### Name

定义predixy服务的名字，在用INFO命令的时候会输出这个内容

例子：

    Name PredixyUserInfo

### Bind

定义predixy服务监听的地址，支持ip:port以及unix socket

例子:

    Bind 0.0.0.0:7617
    Bind /tmp/predixy

未指定时为: 0.0.0.0:7617

### WorkerThreads

指定工作线程数

例子:

    WorkerThreads 4

未指定时为: 1

### MaxMemory

指定predixy最大可申请分配的内存，可以带单位(G/M/K)指定，为0时表示不限制

例子:

    MaxMemory 1024000
    MaxMemory 1G

未指定时为: 0

### ClientTimeout

指定客户端超时时间，以秒为单位，即客户端在空闲时间超过该时间以后将主动断开客户端连接，为0时表示禁止该功能，不主动断开客户端连接

例子:

    ClientTimeout 300

未指定时为: 0

### BufSize

IO Buffer大小，predixy内部分配BufSize大小的缓冲区用于接受客户端命令和服务端响应，完全零拷贝的转发给服务端或者客户端，该值太小的话影响性能，太大的话浪费空间也可能对性能无益。但是具体多少合适要看实际应用场景，predixy默认设该值为4096

例子:

    BufSize 8192

未指定时为: 4096

### Log

指定日志文件名

例子:

    Log /var/log/predixy.log

未指定时predixy的行为是将日志写向标准输出

### LogRotate

日志自动切分选项，可以按时间指定，也可以按文件大小指定，还可以两者都指定。按时间指定支持如下格式：

+ 1d 一天
+ nh 1<= n <= 24    n小时
+ nm 1 <= n <= 1440 n分钟

按文件大小指定支持G和M单位

例子:

    LogRotate 1d    #每天切分一次
    LogRotate 1h    #每小时切分一次
    LogRotate 10m   #每10分钟切分一次
    LogRotate 2G    #日志文件每2G切分一次
    LogRotate 200M  #日志文件每200M切分一次
    LogRotate 1d 2G #每天切分一次，且如果日志文件达到2G也切分一次

未定义时禁用该功能

### LogXXXSample

日志输出采样率，表示每多少条该级别的日志输出一条到Log中，为0时则表示不输出该级别日志。支持的级别如下：

+ LogVerbSample
+ LogDebugSample
+ LogInfoSample
+ LogNoticeSample
+ LogWarnSample
+ LogErrorSample

例子:

    LogVerbSample 0
    LogDebugSample 0
    LogInfoSample 10000
    LogNoticeSample 1
    LogWarnSample 1
    LogErrorSample 1

这几个参数可以在线修改，像redis一样，通过config set命令:

    CONFIG SET LogDebugSample 1

在predixy中，执行config命令需要管理权限


## 权限控制配置部分

predixy扩展了redis中AUTH命令的功能，支持定义多个认证密码，可以为每个密码指定权限，权限包括读权限、写权限和管理权限，其中写权限包括读权限，管理权限又包括写权限。还可以指定每个密码所能读写的健空间，健空间的定义是指健具有某个前缀。

权限控制的定义格式如下：

    Authority {
        Auth [password] {
            Mode read|write|admin
            [KeyPredix Predix...]
            [ReadKeyPredix Predix...]
            [WriteKeyPredix Predix...]
        }...
    }


Authority里面可以定义多个Auth，每个Auth指定一个密码，可以为每个Auth定义权限和健空间。

参数说明：

+ Mode: 必须指定，只能是read、write、admin三者之一，分别表示读、写、管理权限
+ KeyPrefix: 可选项，可以定义健空间，多个健空间用空格隔开
+ ReadKeyPrefix: 可选项，可以定义可读的健空间，多个健空间用空格隔开
+ WriteKeyPrefix: 可选项，可以定义可写的健空间，多个健空间用空格隔开

对于可读的健空间，如果定义了ReadKeyPrefix,则由ReadKeyPrefix决定,否则由KeyPrefix决定，如果两者都没有，则不限制。可写的健空间解释也一样。需要注意的是，有写权限就表示有读权限，但是读写健空间是完全独立的，即WriteKeyPrefix不会默认包括ReadKeyPrefix的内容。

例子:

    Authority {
        Auth {
            Mode read
            KeyPrefix Info
        }
        Auth readonly {
            Mode read
        }
        Auth modify {
            Mode write
            ReadPrefix User Stats
            WritePrefix User
        }
    }

上面的例子定义了三个认证密码

+ 空密码，因为密码为空，所以这个认证是默认的认证，它具有读权限，由于指定了KeyPrefix，因此它最终的权限是只能读取具有前缀Info的key
+ readonly密码，这个认证具有读权限，没有健空间限制，它可以读所有key
+ modify密码，这个认证具有写权限，分别定义了可读健空间User和Stats,因此它能读取具有这两个前缀的key，而可写健空间定义为User,因此它能写具有前缀User的key，但是不能写具有前缀Stats的key

缺省的predixy权限控制定义如下：

    Authority {
        Auth {
            Mode write
        }
        Auth "#a complex password#" {
            Mode admin
        }
    }

它表示无需密码即可读写所有的key，但是管理权限要求输入密码#a complex password#

## redis实例配置部分

predixy支持Redis Sentinel和Redis Cluster来使用redis，一个配置里这两种形式只能出现一种。

### Redis Sentinel形式

定义格式如下：

    SentinelServerPool {
        [Password xxx]
        [Databases number]
        Hash atol|crc16
        [HashTag "xx"]
        Distribution modula|random
        [MasterReadPriority [0-100]]
        [StaticSlaveReadPriority [0-100]]
        [DynamicSlaveReadPriority [0-100]]
        [RefreshInterval number[s|ms|us]]
        [ServerTimeout number[s|ms|us]]
        [ServerFailureLimit number]
        [ServerRetryTimeout number[s|ms|us]]
        [KeepAlive seconds]
        Sentinels {
            + addr
            ...
        }
        Group xxx {
            [+ addr]
            ...
        }
    }

参数说明：

+ Password: 指定连接redis实例默认的密码，不指定的情况下表示redis不需要密码
+ Databases: 指定redis db数量，不指定的情况下为1
+ Hash: 指定对key算哈希的方法，当前只支持atol和crc16
+ HashTag: 指定哈希标签，不指定的话为{}
+ Distribution: 指定分布key的方法，当前只支持modula和random
+ MasterReadPriority: 读写分离功能，从redis master节点执行读请求的优先级，为0则禁止读redis master，不指定的话为50
+ StaticSlaveReadPriority: 读写分离功能，从静态redis slave节点执行读请求的优先级，所谓静态节点，是指在本配置文件中显示列出的redis节点，不指定的话为0
+ DynamicSlaveReadPolicy: 功能见上，所谓动态节点是指在本配置文件中没有列出，但是通过redis sentinel动态发现的节点，不指定的话为0
+ RefreshInterval: predixy会周期性的请求redis sentinel以获取最新的集群信息，该参数以秒为单位指定刷新周期，不指定的话为1秒
+ ServerTimeout: 请求在predixy中最长的处理/等待时间，如果超过该时间redis还没有响应的话，那么predixy会关闭同redis的连接，并给客户端一个错误响应，对于blpop这种阻塞式命令，该选项不起作用，为0则禁止此功能，即如果redis不返回就一直等待，不指定的话为0
+ ServerFailureLimit: 一个redis实例出现多少次才错误以后将其标记为失效，不指定的话为10
+ ServerRetryTimeout: 一个redis实例失效后多久后去检查其是否恢复正常，不指定的话为1秒
+ KeepAlive: predixy与redis的连接tcp keepalive时间，为0则禁止此功能，不指定的话为0
+ Sentinels: 里面定义redis sentinel实例的地址
+ Group: 定义一个redis组，Group的名字应该和redis sentinel里面的名字一致，Group里可以显示列出redis的地址，列出的话就是上面提到的静态节点

一个例子：

    SentinelServerPool {
        Databases 16
        Hash crc16
        HashTag "{}"
        Distribution modula
        MasterReadPriority 60
        StaticSlaveReadPriority 50
        DynamicSlaveReadPriority 50
        RefreshInterval 1
        ServerTimeout 1
        ServerFailureLimit 10
        ServerRetryTimeout 1
        KeepAlive 120
        Sentinels {
            + 10.2.2.2:7500
            + 10.2.2.3:7500
            + 10.2.2.4:7500
        }
        Group shard001 {
        }
        Group shard002 {
        }
    }

这个Redis Sentinel集群定义指定了三个redis sentinel实例，分别是10.2.2.2:7500、10.2.2.3:7500、10.2.2.4:7500，定义了两组redis，分别是shard001、shard002。没有指定任何静态redis节点。所有redis实例都没有开启密码认证，它们都具有16个db。predixy用crc16计算key的哈希值，然后通过modula也就是求模的办法将key分布到shard001或shard002中去。由于MasterReadPriority为60,比DynamicSlaveReadPriority的50要大，所以读请求都会分发到redis master节点，RefreshInterval为1,每一秒钟向redis sentinel发送请求刷新集群信息。redis实例失败累计达到10次后将该redis实例标记失效，每间隔1秒钟后检查其是否恢复。

### Redis Cluster形式

定义格式如下：

    ClusterServerPool {
        [Password xxx]
        [MasterReadPriority [0-100]]
        [StaticSlaveReadPriority [0-100]]
        [DynamicSlaveReadPriority [0-100]]
        [RefreshInterval seconds]
        [ServerTimeout number[s|ms|us]]
        [ServerFailureLimit number]
        [ServerRetryTimeout number[s|ms|us]]
        [KeepAlive seconds]
        Servers {
            + addr
            ...
        }
    }


参数说明：

+ 可选的参数和Redis Sentinel模式同名参数含义一样
+ Servers:在这里列出redis cluster里的redis实例，列出的为静态节点，未列出而是通过集群信息发现的则为动态节点

一个例子：

    ClusterServerPool {
        MasterReadPriority 0
        StaticSlaveReadPriority 50
        DynamicSlaveReadPriority 50
        RefreshInterval 1
        ServerTimeout 1
        ServerFailureLimit 10
        ServerRetryTimeout 1
        KeepAlive 120
        Servers {
            + 192.168.2.107:2211
            + 192.168.2.107:2212
        }
    }

该定义指定通过redis实例192.168.2.107:2211和192.168.2.107:2212来发现集群信息，指定MasterReadPriority为0，表示不要将读请求分发到redis master节点。


## 多数据中心配置部分

predixy支持多数据中心，在redis部署跨数据中心的时候，可以将读请求分发给本数据中心的redis实例，避免跨数据中心访问。套用数据中心的概念，实际上即便没有多数据中心，也可以在需要从节点来分担读请求的时候通过本配置来控制请求分配，此时比如可以认为一个机架就是一个数据中心。

多数据中心配置格式:

    LocalDC name
    DataCenter {
       DC name {
           AddrPrefix {
               + IpPrefix
               ...
           }
           ReadPolicy {
               name priority [weight]
               other priority [weight]
           }
       }
       ...
    }


参数说明：

+ LocalDC: 指定当前predixy所在的数据中心
+ DC: 定义一个数据中心
+ AddrPrefix: 定义该数据中心包括的ip前缀
+ ReadPolicy: 定义从该数据中心读其它（包括自己）数据中心的优先级及权重

如果不用数据中心功能，那么不提供LocalDC和DataCenter定义即可。

一个多数据中心配置的例子:

    DataCenter {
        DC bj {
            AddrPrefix {
                + 10.1
            }
            ReadPolicy {
                bj 50
                sh 20
                sz 10
            }
        }
        DC sh {
            AddrPrefix {
                + 10.2
            }
            ReadPolicy {
                sh 50
                bj 20 5
                sz 20 2
            }
        }
        DC sz {
            AddrPrefix {
                + 10.3
            }
            ReadPolicy {
                sz 50
                sh 20
                bj 10
            }
        }
    }

这个例子定义了三个数据中心，分别是bj、sh、sz。其中bj数据中心包括ip前缀为10.1的redis实例，这样predixy通过redis sentinel或者redis cluster发现节点的时候如果看到redis实例的地址前缀是10.1则会认为该实例在bj数据中心。predixy根据自身所在的数据中心来选择相应的读请求策略。

假设predixy在bj数据中心，bj读bj的优先级为50,高于另外两个，所以predixy会优先选择bj数据中心进行读操作，如果bj数据中心没有可用的redis节点，则会是sh数据中心，如果sh还没有节点，才会选择sz。

假设predixy在sh数据中心，predixy优先选择sh数据中心，如果sh数据中心没有可用的redis实例，因为bj和sh的优先级都为20,那么则会根据权重设置来分配流量，在这里，5份的请求去bj数据中心，2份的请求去sz。

前面在定义集群的时候有说过可以定义主从节点的读优先级，数据中心这里又有读优先级的概念，那么它们是如何工作的？原则就是在启用了数据中心的功能之后，先根据数据中心读策略选取数据中心、然后再在数据中心内应用集群的主从读优先级选择最终的redis实例。


## 延迟监控配置部分

predixy提供了强大的延迟监控的功能，可以记录predixy处理请求的时间，对predixy来说，这个时间其实主要就是请求redis的时间。

延迟监控定义格式如下：

    LatencyMonitor name {
        Commands {
            + cmd
            [- cmd]
            ...
        }
        TimeSpan {
            + TimeElapsedUS
            ...
        }
    }

参数说明:

+ LatencyMonitor: 定义一个延迟监控
+ Commands: 指定该延迟监控记录哪些redis命令，+ cmd表示监控该命令，- cmd表示不监控该命令，如果cmd为all则表示所有命令。
+ TimeSpan: 定义延迟桶，以微秒为单位，必须是一个严格递增的序列。

可以定义多个LatencyMonitor以便监控不同的命令。

延迟监控配置例子：

    LatencyMonitor all {
        Commands {
            + all
            - blpop
            - brpop
            - brpoplpush
        }
        TimeSpan {
            + 1000
            + 1200
            + 1400
            + 1600
            + 1700
            + 1800
            + 2000
            + 2500
            + 3000
            + 3500
            + 4000
            + 4500
            + 5000
            + 6000
            + 7000
            + 8000
            + 9000
            + 10000
        }
    }

    LatencyMonitor get {
        Commands {
            + get
        }
        TimeSpan {
            + 100
            + 200
            + 300
            + 400
            + 500
            + 600
            + 700
            + 800
            + 900
            + 1000
        }
    }

    LatencyMonitor set {
        Commands {
            + set
            + setnx
            + setex
        }
        TimeSpan {
            + 100
            + 200
            + 300
            + 400
            + 500
            + 600
            + 700
            + 800
            + 900
            + 1000
        }
    }


上面的例子定义了三个延迟监控，其中all监控除了blpop/brpop/brpoplpush外的所有命令，get监控get命令，set监控set/setnx/setex命令。这里的耗时桶定义并不适用与所有情况，在实际使用的时候需要调整以更精确的反应真实的耗时情况。

查看耗时监控，通过INFO命令来查看，有三种使用方式：

### 查看所有延迟定义的整体信息

命令：

    redis> INFO

此时看到的结果列在最后的就是整体的延迟信息。

### 查看单个延迟定义的信息

命令：

    redis> INFO Latency <name>

其中<name>为延迟定义名称，结果会显示该定义的整体延迟信息，以及分后端redis实例的延迟信息。

### 查看某个后端redis实例延迟信息
命令：

    redis> INFO ServerLatency <serv> [name]

其中<serv>为redis实例地址，[name]为可选的延迟定义名称，如果忽略name，那么就会给出请求该redis实例所有延迟定义信息，否则只给出name的。

延迟信息格式说明，下面是延迟定义all的整体信息一个例子：

    LatencyMonitorName:all
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

+ 第一列为<=，则第二列表示小于等于该耗时，第三列表示这个范围耗时的总和，第四列表示请求数，第五列表示累计的请求占总请求的百分比，
+ 第一列为>，则第二列表示大于该耗时的请求，后两列含义同上。本行最多只会出现一次，如果位于本行的请求数过多，则说明延迟定义指定的耗时桶不够合适。
+ 第一列为T，这一行只会出现一次，且总在最后。第二列表示所有请求的平均耗时，第三列表示总的请求耗时之和，第四列表示总的请求数。
