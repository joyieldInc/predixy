## redis cluster server pool define

##ClusterServerPool {
##    [Password xxx]                        #default no
##    [MasterReadPriority [0-100]]          #default 50
##    [StaticSlaveReadPriority [0-100]]     #default 0
##    [DynamicSlaveReadPriority [0-100]]    #default 0
##    [RefreshInterval number[s|ms|us]]     #default 1, means 1 second
##    [ServerTimeout number[s|ms|us]]       #default 0, server connection socket read/write timeout
##    [ServerFailureLimit number]           #default 10
##    [ServerRetryTimeout number[s|ms|us]]  #default 1
##    [KeepAlive seconds]                   #default 0, server connection tcp keepalive

##    Servers {
##        + addr
##        ...
##    }
##}


## Examples:
#ClusterServerPool {
#    MasterReadPriority 60
#    StaticSlaveReadPriority 50
#    DynamicSlaveReadPriority 50
#    RefreshInterval 1
#    ServerTimeout 1
#    ServerFailureLimit 10
#    ServerRetryTimeout 1
#    KeepAlive 120
#    Servers {
#        + 192.168.2.107:2211
#        + 192.168.2.107:2212
#    }
#}

