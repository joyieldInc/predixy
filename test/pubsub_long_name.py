#!/usr/bin/env python3
#
# Verify subscribe/psubscribe confirmation with long channel/pattern names
#

import sys
from test_util import parse_args, make_client, exit_with_result


def normalize_bytes(value):
    if isinstance(value, bytes):
        return value.decode("utf-8")
    return value


def run_test(host, port):
    long_name = "ch_" + ("x" * 200)
    pattern = long_name + "*"

    c1 = make_client(host, port)
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
    args = parse_args("Long pubsub name test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "long pubsub name", "long pubsub name")
