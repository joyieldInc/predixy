#!/usr/bin/env python3
#
# Verify pubsub parser state does not reuse old messages
#

import sys
from test_util import parse_args, make_clients, exit_with_result


def run_test(host, port):
    c1, c2 = make_clients(host, port, count=2)

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
    args = parse_args("Pubsub parser reset test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "pubsub parser reset",
                     "pubsub parser reset")
