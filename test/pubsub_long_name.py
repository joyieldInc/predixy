#!/usr/bin/env python3
#
# Verify subscribe/psubscribe confirmation with long channel/pattern names
#

import argparse
import sys
import redis


def normalize_bytes(value):
    if isinstance(value, bytes):
        return value.decode("utf-8")
    return value


def run_test(host, port):
    long_name = "ch_" + ("x" * 200)
    pattern = long_name + "*"

    c1 = redis.StrictRedis(host=host, port=port)
    ps = c1.pubsub()

    ps.subscribe(long_name)
    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "subscribe":
        print("FAIL: subscribe confirmation missing:", msg)
        return False

    if normalize_bytes(msg.get("channel")) != long_name:
        print("FAIL: subscribe channel mismatch:", msg)
        return False

    if msg.get("data") != 1:
        print("FAIL: subscribe count mismatch:", msg)
        return False

    ps.psubscribe(pattern)
    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "psubscribe":
        print("FAIL: psubscribe confirmation missing:", msg)
        return False

    if normalize_bytes(msg.get("channel")) != pattern:
        print("FAIL: psubscribe channel mismatch:", msg)
        return False

    if msg.get("data") != 2:
        print("FAIL: psubscribe count mismatch:", msg)
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="Long pubsub name test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: long pubsub name")
        sys.exit(0)
    print("FAIL: long pubsub name")
    sys.exit(1)
