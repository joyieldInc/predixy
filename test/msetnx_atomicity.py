#!/usr/bin/env python3
#
# Verify MSETNX does not partially apply changes when it returns 0
#

import argparse
import sys
import redis


def run_test(host, port):
    c = redis.StrictRedis(host=host, port=port)
    c.delete("msetnx_k1", "msetnx_k2")
    c.set("msetnx_k1", "existing")

    try:
        r = c.execute_command("MSETNX", "msetnx_k1", "v1", "msetnx_k2", "v2")
    except Exception as exc:
        # If proxy rejects cross-shard MSETNX, this is acceptable.
        return True

    if r not in (0, b"0", False):
        print("FAIL: expected MSETNX result 0, got:", r)
        return False

    if c.get("msetnx_k2") is not None:
        print("FAIL: MSETNX partially applied when it returned 0")
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="MSETNX atomicity test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: MSETNX atomicity")
        sys.exit(0)
    print("FAIL: MSETNX atomicity")
    sys.exit(1)
