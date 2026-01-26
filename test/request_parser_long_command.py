#!/usr/bin/env python3
#
# Send an overlong command name to ensure parser doesn't crash.
#

import socket
from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    long_cmd = b"a" * 32
    payload = b"*2\r\n$32\r\n" + long_cmd + b"\r\n$1\r\nx\r\n"
    try:
        sock = socket.create_connection((host, port), timeout=1.0)
        sock.sendall(payload)
        sock.close()
    except Exception as exc:
        print("WARN: long command send failed:", exc)

    try:
        c = make_client(host, port)
        if c.ping() is not True:
            print("FAIL: ping after long command")
            return False
    except Exception as exc:
        print("FAIL: ping after long command:", exc)
        return False
    return True


if __name__ == "__main__":
    args = parse_args("Request parser long command test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "request parser long command", "request parser long command")
