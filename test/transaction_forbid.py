#!/usr/bin/env python
#
# Verify forbidden command in transaction returns error without closing connection
#

import argparse
import sys
import redis


def run_test(host, port):
    c = redis.StrictRedis(host=host, port=port)
    try:
        r = c.execute_command("MULTI")
        if r not in (b"OK", "OK"):
            print("FAIL: MULTI response:", r)
            return False
    except Exception as exc:
        print("FAIL: MULTI error:", exc)
        return False

    try:
        c.execute_command("SELECT", "0")
        print("FAIL: SELECT should be forbidden in transaction")
        return False
    except Exception:
        pass

    try:
        r = c.execute_command("PING")
        if r not in (b"PONG", "PONG", True):
            print("FAIL: PING after error:", r)
            return False
    except Exception as exc:
        print("FAIL: PING after error exception:", exc)
        return False

    try:
        c.execute_command("DISCARD")
    except Exception:
        pass

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="Transaction forbid test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: transaction forbid")
        sys.exit(0)
    print("FAIL: transaction forbid")
    sys.exit(1)
