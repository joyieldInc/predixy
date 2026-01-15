#!/usr/bin/env python3
#
# Verify subscription confirmations arrive before messages
#

import sys
from test_util import parse_args, make_clients, exit_with_result
import time


def run_test(host, port):
    c1, c2 = make_clients(host, port, count=2)

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
    args = parse_args("Pubsub subscription order test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "pubsub subscription order",
                     "pubsub subscription order")
