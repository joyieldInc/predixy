#!/usr/bin/env python
#
# Verify subscription confirmations arrive before messages
#

import argparse
import sys
import redis
import time


def run_test(host, port):
    c1 = redis.StrictRedis(host=host, port=port)
    c2 = redis.StrictRedis(host=host, port=port)

    ps = c1.pubsub()
    ps.subscribe("ch_order")

    # Publish quickly after subscribe to stress ordering.
    c2.publish("ch_order", "order_msg")

    msgs = []
    for _ in range(5):
        msg = ps.get_message(timeout=0.5)
        if msg:
            msgs.append(msg)
        if len(msgs) >= 2:
            break

    if not msgs:
        print("FAIL: missing subscribe confirmation")
        return False

    if msgs[0].get("type") != "subscribe":
        print("FAIL: first message is not subscribe:", msgs[0])
        return False

    if len(msgs) > 1 and msgs[1].get("type") != "message":
        print("FAIL: second message is not data message:", msgs[1])
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="Pubsub subscription order test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: pubsub subscription order")
        sys.exit(0)
    print("FAIL: pubsub subscription order")
    sys.exit(1)
