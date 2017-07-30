## LatencyMonitor record command time eplapsed
## redis command INFO will show the latency monitor results
##
## see predixy info, include latency monitor for predixy
## redis> INFO
##
## see latency monitor for specify latency name
## redis> INFO Latency name
##
## see latency monitor for specify server
## redis> INFO ServerLatency ServAddr [name]
##
## reset all stats info, include latency monitor, require admin permission
## redis> CONFIG ResetStat
##
## Examples:
## LatencyMonitor name {
##     Commands {
##         + cmd
##         [- cmd]
##         ...
##     }
##     TimeSpan {
##         + TimeElapsedUS
##         ...
##     }
## }
## cmd is redis commands, "all" means all commands

LatencyMonitor all {
    Commands {
        + all
        - blpop
        - brpop
        - brpoplpush
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

LatencyMonitor blist {
    Commands {
        + blpop
        + brpop
        + brpoplpush
    }
    TimeSpan {
        + 1000
        + 2000
        + 3000
        + 4000
        + 5000
        + 6000
        + 7000
        + 8000
        + 9000
        + 10000
        + 20000
        + 30000
        + 40000
        + 50000
        + 60000
        + 70000
        + 80000
        + 90000
        + 100000
    }
}
