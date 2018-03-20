## Configuration example
cluster.conf 

    ClusterServerPool {
        MasterReadPriority 60
        StaticSlaveReadPriority 50
        DynamicSlaveReadPriority 50
        RefreshInterval 300ms
        ServerTimeout 300ms                 #added in commit 86183a4
        ServerFailureLimit 2
        ServerRetryTimeout 500ms
        KeepAlive 120s                      #added in commit 86183a4
        Servers {
            + 192.168.2.107:2211
            + 192.168.2.108:2212
            + 192.168.2.109:2213
        }
    }

## Configuration parameters description

**MasterReadPriority, StaticSlaveReadPriority** and **DynamicSlaveReadPriority** - these parameters work in conjunction only in predixy and have _nothing_ with Redis configuration directives like slave-priority. As predixy can work both with Redis Sentinel or Redis Cluster deployments, two options can be used: MasterReadPriority and StaticSlaveReadPriority, or MasterReadPriority and DynamicSlaveReadPriority.

If your Redis deployment implies nodes auto-discovery with Sentinel or other cluster nodes, the DynamicSlaveReadPriority option will be used; if you plan to add nodes in predixy config to **Servers {...}** manually, StaticSlaveReadPriority will be used.

In other words, predixy can discover automatically added Redis-related nodes polling existing **Servers {...}** and also route queries to them, eliminating the need of manually editing of cluster.conf and restarting the predixy.

These three parameters tell predixy in what way and proportion it should route queries to available nodes. 

Some use cases you can see below:

| Master/SlaveReadPriority  | Master | Slave1 | Slave2 | Fail-over notes  |
| ------------- | ------------- | ------------- | ------------- | ------------- |
| 60/50  |  	all requests  | 0 requests | 0 requests | Master dead, read requests deliver to slave until master(maybe new master) alive |
| 60/0  |  	all requests  | 0 requests | 0 requests | Master dead, all requests fail |
| 50/50  |  	all write requests, 33.33%read requests  | 33.33% read requests | 33.33% read requests | - |
| 0/50  |  	all write requests, 0 read requests  | 50% read requests | 50% read requests | all slaves dead, all read requests fail |
| 10/50  |  	all write requests, 0 read requests  | 50% read requests | 50% read requests | all slaves dead, read requests deliver to master |

**RefreshInterval** - seconds, milliseconds or microseconds [s|ms|us], tells predixy how often it should poll nodes info and allocated hash slots in case of Cluster usage

**ServerFailureLimit** - amount of failed queries to node when predixy stop forwarding queries to this node

**ServerTimeout** - seconds, milliseconds or microseconds [s|ms|us], is timeout for nearly all commands except BRPOP, BLPOP, BRPOPLUSH, transactions and PUB/SUB. It's Redis server connection socket read/write timeout, to avoid wait for a Redis response too long. When timeout reached, Predixy will close connection between itself and Redis and throw an error.
**Available from the commit 86183a4.**

**KeepAlive** - seconds, applied to redis connection(socket), it works for all commands that not covered with the **ServerTimeout**, i.e. applied also to commands with blocking nature like BRPOP, BLPOP, BRPOPLPUSH. hen timeout reached, Predixy will close connection between itself and Redis and throw an error.
**Available from the commit 86183a4.**

**ServerRetryTimeout** - seconds, milliseconds or microseconds [s|ms|us], tells predixy how often it should try to check health of failed nodes and decide if they still failed or alive and can be used for queries processing

**Servers** - just line-by-line list of static Redis nodes in socket fashion, like `+ IP:PORT`
