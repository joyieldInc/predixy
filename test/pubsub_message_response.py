#!/usr/bin/env python3
#
# Verify pubsub message responses include message data
#

from test_util import parse_args, make_clients, exit_with_result


def normalize_bytes(value):
    if isinstance(value, bytes):
        return value.decode("utf-8")
    return value


def run_test(host, port):
    c1, c2 = make_clients(host, port, count=2)

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
    args = parse_args("Pubsub message response test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "pubsub message response",
                     "pubsub message response")
