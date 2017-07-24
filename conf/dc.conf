## DataCenter 
## DataCenter {
##    DC name {
##        AddrPrefix {
##            + IpPrefix
##            ...
##        }
##        ReadPolicy {
##            name priority [weight]
##            other priority [weight]
##        }
##    }
##    ...
## }
## Examples:
#DataCenter {
#    DC bj {
#        AddrPrefix {
#            + 10.1
#        }
#        ReadPolicy {
#            bj 50
#            sh 20
#            sz 10
#        }
#    }
#    DC sh {
#        AddrPrefix {
#            + 10.2
#        }
#        ReadPolicy {
#            sh 50
#            bj 20 5
#            sz 20 2
#        }
#    }
#    DC sz {
#        AddrPrefix {
#            + 10.3
#        }
#        ReadPolicy {
#            sz 50
#            sh 20
#            bj 10
#        }
#    }
#}
