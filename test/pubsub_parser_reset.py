#!/usr/bin/env python
#
# Verify pubsub parser state does not reuse old messages
#

import argparse
import sys
import redis


def run_test(host, port):
    c1 = redis.StrictRedis(host=host, port=port)
    c2 = redis.StrictRedis(host=host, port=port)

    ps = c1.pubsub()
    ps.subscribe("ch_reset")
    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "subscribe":
        print("FAIL: subscribe confirmation missing:", msg)
        return False

    c2.publish("ch_reset", "first")
    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "message":
        print("FAIL: missing first message:", msg)
        return False

    ps.psubscribe("ch_reset*")
    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "psubscribe":
        print("FAIL: expected psubscribe confirmation, got:", msg)
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="Pubsub parser reset test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: pubsub parser reset")
        sys.exit(0)
    print("FAIL: pubsub parser reset")
    sys.exit(1)
