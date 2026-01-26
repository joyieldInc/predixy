#!/usr/bin/env python3
#
# Trigger parser errors at the first byte to verify safe error logging.
#

import socket
from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    try:
        sock = socket.create_connection((host, port), timeout=1.0)
        sock.sendall(b"!")
        sock.close()
    except Exception as exc:
        print("WARN: invalid byte send failed:", exc)

    try:
        c = make_client(host, port)
        if c.ping() is not True:
            print("FAIL: ping after invalid byte")
            return False
    except Exception as exc:
        print("FAIL: ping after invalid byte:", exc)
        return False
    return True


if __name__ == "__main__":
    args = parse_args("Request parser error logging test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "request parser error logging", "request parser error logging")
