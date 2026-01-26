#!/usr/bin/env python3
#
# Ensure large pubsub payloads are parsed correctly.
#

from test_util import parse_args, make_clients, exit_with_result


def normalize_bytes(value):
    if isinstance(value, bytes):
        return value.decode("utf-8")
    return value


def wait_for_type(ps, msg_type, attempts=20, timeout=1.0):
    for _ in range(attempts):
        msg = ps.get_message(timeout=timeout)
        if msg and msg.get("type") == msg_type:
            return msg
    return None


def run_test(host, port):
    c1, c2 = make_clients(host, port, count=2)

    ps = c1.pubsub()
    ps.subscribe("big_payload")
    msg = wait_for_type(ps, "subscribe")
    if not msg:
        print("FAIL: missing subscribe confirmation")
        return False

    payload = "x" * 10000
    publish_result = c2.publish("big_payload", payload)
    if publish_result < 1:
        print("FAIL: publish did not reach subscribers:", publish_result)
        return False

    msg = wait_for_type(ps, "message", attempts=30, timeout=1.0)
    if not msg:
        print("FAIL: missing message response")
        return False

    data = normalize_bytes(msg.get("data"))
    if data != payload:
        print("FAIL: payload mismatch (len)", len(data) if data else 0)
        return False

    return True


if __name__ == "__main__":
    args = parse_args("Pubsub large message test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "pubsub large message",
                     "pubsub large message")
