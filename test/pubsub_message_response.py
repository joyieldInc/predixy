#!/usr/bin/env python3
#
# Verify pubsub message responses include message data
#

import argparse
import sys
import redis


def normalize_bytes(value):
    if isinstance(value, bytes):
        return value.decode("utf-8")
    return value


def run_test(host, port):
    c1 = redis.StrictRedis(host=host, port=port)
    c2 = redis.StrictRedis(host=host, port=port)

    ps = c1.pubsub()
    ps.subscribe("ch_resp")
    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "subscribe":
        print("FAIL: missing subscribe confirmation:", msg)
        return False

    publish_result = c2.publish("ch_resp", "hello_resp")
    if publish_result < 1:
        print("FAIL: publish did not reach subscribers:", publish_result)
        return False

    msg = ps.get_message(timeout=1.0)
    if not msg or msg.get("type") != "message":
        print("FAIL: missing message response:", msg)
        return False

    data = normalize_bytes(msg.get("data"))
    if data != "hello_resp":
        print("FAIL: unexpected message data:", msg)
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="Pubsub message response test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: pubsub message response")
        sys.exit(0)
    print("FAIL: pubsub message response")
    sys.exit(1)
