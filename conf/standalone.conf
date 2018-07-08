## redis standalone server pool define

##StandaloneServerPool {
##    [Password xxx]                        #default no
##    [Databases number]                    #default 1
##    Hash atol|crc16
##    [HashTag "xx"]                        #default no
##    Distribution modula|random
##    [MasterReadPriority [0-100]]          #default 50
##    [StaticSlaveReadPriority [0-100]]     #default 0
##    [DynamicSlaveReadPriority [0-100]]    #default 0
##    RefreshMethod fixed|sentinel          #
##    [RefreshInterval number[s|ms|us]]     #default 1, means 1 second
##    [ServerTimeout number[s|ms|us]]       #default 0, server connection socket read/write timeout
##    [ServerFailureLimit number]           #default 10
##    [ServerRetryTimeout number[s|ms|us]]  #default 1
##    [KeepAlive seconds]                   #default 0, server connection tcp keepalive
##    Sentinels [sentinel-password] {
##        + addr
##        ...
##    }
##    Group xxx {
##        [+ addr]                          #if RefreshMethod==fixed: the first addr is master in a group, then all addrs is slaves in this group
##        ...
##    }
##}


## Examples:
#StandaloneServerPool {
#    Databases 16
#    Hash crc16
#    HashTag "{}"
#    Distribution modula
#    MasterReadPriority 60
#    StaticSlaveReadPriority 50
#    DynamicSlaveReadPriority 50
#    RefreshMethod sentinel
#    RefreshInterval 1
#    ServerTimeout 1
#    ServerFailureLimit 10
#    ServerRetryTimeout 1
#    KeepAlive 120
#    Sentinels {
#        + 10.2.2.2:7500
#        + 10.2.2.3:7500
#        + 10.2.2.4:7500
#    }
#    Group shard001 {
#    }
#    Group shard002 {
#    }
#}

#StandaloneServerPool {
#    Databases 16
#    Hash crc16
#    HashTag "{}"
#    Distribution modula
#    MasterReadPriority 60
#    StaticSlaveReadPriority 50
#    DynamicSlaveReadPriority 50
#    RefreshMethod fixed
#    ServerTimeout 1
#    ServerFailureLimit 10
#    ServerRetryTimeout 1
#    KeepAlive 120
#    Group shard001 {
#       + 10.2.3.2:6379
#    }
#}
