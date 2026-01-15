#!/usr/bin/env python3
#
# Exercise server write mismatch handling to ensure proxy stays alive
#

import socket
import sys
import time
from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    # Send a malformed pipelined request and close quickly.
    try:
        sock = socket.create_connection((host, port), timeout=1.0)
        sock.sendall(b"*2\r\n$4\r\nping\r\n$4\r\nping\r\n")
        sock.close()
    except Exception as exc:
        print("WARN: socket setup failed:", exc)

    # Ensure the proxy still accepts connections.
    try:
        c = make_client(host, port)
        if c.ping() is not True:
            print("FAIL: ping did not return True")
            return False
    except Exception as exc:
        print("FAIL: ping after malformed request:", exc)
        return False

    return True


if __name__ == "__main__":
    args = parse_args("Null response handling test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "null response handling",
                     "null response handling")
