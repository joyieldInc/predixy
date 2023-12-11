# 7.0.1

+ Add new config IgnoreArgNumCheck, use to ignore argument number check in Predixy
+ Improve the performance for relaod license
+ Fix the rewrite error response

# 7.0.0

+ Support new command for Redis 6.x and 7.x, list below:
+ Function: all Function serial commands
+ Script: eval_ro; improve script implement
+ Set: sintercard smismember
+ Zset: bzmpop zdiff zdiffstore zinter zintercard zmpop zmscore zrandmember
zrangestore zunion
+ List: lmove lmpop lpos
+ Geo: georadius_ro georadiusbymember_ro geosearch geosearchstore
+ Other: reset expiretime pexiretime sort_ro bitfield_ro getdel getex
lcs substr hrandfield
+ PubSub: supports Shard PubSub; refactor all PubSub codes for Predixy, now 
Predixy can support Redis PubSub fully.

# 5.2.0

+ Supports domain in config for discover redis nodes
+ Auto remove the redis node if it's not stay in the Redis Cluster
+ Improve the deploy for exporter
+ Require license to running
+ Improve the compatibility for client within Redis Cluster
+ Disable some redis commands in config
+ PINFO reply the arch information
+ WorkerThreads: the config field can auto detect the CPU cores
