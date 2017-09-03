#!/usr/bin/env python
# 
#  predixy - A high performance and full features proxy for redis.
#  Copyright (C) 2017 Joyield, Inc. <joyield.com@gmail.com>
#  All rights reserved.

import time
import redis
import sys
import argparse

c1 = None
c2 = None

def test():
    ps = c1.pubsub()
    stats = [
        [ps, 'subscribe', ['ch']],
        [ps, 'get_message', [], {'pattern': None, 'type': 'subscribe', 'channel': 'ch', 'data': 1L}],
        [c2, 'publish', ['ch', 'hello'], 1],
        [ps, 'get_message', [], {'pattern': None, 'type': 'message', 'channel': 'ch', 'data': 'hello'}],
        [ps, 'subscribe', ['ch1', 'ch2']],
        [ps, 'get_message', [], {'pattern': None, 'type': 'subscribe', 'channel': 'ch1', 'data': 2L}],
        [ps, 'get_message', [], {'pattern': None, 'type': 'subscribe', 'channel': 'ch2', 'data': 3L}],
        [c2, 'publish', ['ch1', 'channel1'], lambda x:True],
        [c2, 'publish', ['ch2', 'channel2'], lambda x:True],
        [ps, 'get_message', [], {'pattern': None, 'type': 'message', 'channel': 'ch1', 'data': 'channel1'}],
        [ps, 'get_message', [], {'pattern': None, 'type': 'message', 'channel': 'ch2', 'data': 'channel2'}],
        [ps, 'psubscribe', ['ch*']],
        [ps, 'get_message', [], {'pattern': None, 'type': 'psubscribe', 'channel': 'ch*', 'data': 4L}],
        [c2, 'publish', ['ch', 'hello'], 2],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['data']=='hello'],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['data']=='hello'],
        [ps, 'psubscribe', ['ch1*', 'ch2*']],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='psubscribe'],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='psubscribe'],
        [ps, 'unsubscribe', ['ch']],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='unsubscribe'],
        [c2, 'publish', ['ch', 'hello'], 1],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['data']=='hello'],
        [ps, 'punsubscribe', ['ch*']],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='punsubscribe'],
        [ps, 'unsubscribe', ['ch1', 'ch2']],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='unsubscribe'],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='unsubscribe'],
        [ps, 'punsubscribe', ['ch1*', 'ch2*']],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='punsubscribe'],
        [ps, 'get_message', [], lambda x:type(x)==type({}) and x['type']=='punsubscribe'],
    ]
    def run(stat):
        func = getattr(stat[0], stat[1])
        r = func(*stat[2])
        if len(stat) == 3:
            print('EXEC %s(*%s)' % (stat[1], repr(stat[2])))
            return True
        if hasattr(stat[3], '__call__'):
            isPass = stat[3](r)
        else:
            isPass = r == stat[3]
        if isPass:
            print('PASS %s(*%s):%s' % (stat[1], repr(stat[2]), repr(r)))
            return True
        else:
            print('FAIL %s(*%s):%s != %s' % (stat[1], repr(stat[2]), repr(r), repr(stat[3])))
            return False

    succ = True
    for stat in stats:
        if not run(stat):
            succ = False
        time.sleep(0.2)
    print '---------------------------------'
    if succ:
        print 'Good! PubSub test pass'
    else:
        print 'Oh! PubSub some case fail'



if __name__ == '__main__':
    parser = argparse.ArgumentParser(conflict_handler='resolve')
    parser.add_argument('-h', nargs='?', default='127.0.0.1', help='host')
    parser.add_argument('-p', nargs='?', default=7617, type=int, help='port')
    args = parser.parse_args()
    host = '127.0.0.1' if not args.h else args.h
    port = 7617 if not args.p else args.p
    c1 = redis.StrictRedis(host=host, port=port)
    c2 = redis.StrictRedis(host=host, port=port)
    test()


