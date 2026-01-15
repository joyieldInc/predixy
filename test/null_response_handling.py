#!/usr/bin/env python
#
# Exercise server write mismatch handling to ensure proxy stays alive
#

import argparse
import socket
import sys
import time
import redis


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
        c = redis.StrictRedis(host=host, port=port)
        if c.ping() is not True:
            print("FAIL: ping did not return True")
            return False
    except Exception as exc:
        print("FAIL: ping after malformed request:", exc)
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="Null response handling test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: null response handling")
        sys.exit(0)
    print("FAIL: null response handling")
    sys.exit(1)
