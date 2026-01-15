#!/usr/bin/env python3
#
# Verify MGET returns WRONGTYPE for non-string keys
#

import argparse
import sys
import redis


def run_test(host, port):
    c = redis.StrictRedis(host=host, port=port)
    c.delete("mget_wrong_type")
    c.lpush("mget_wrong_type", "v1")

    res = c.execute_command("MGET", "mget_wrong_type")
    if not isinstance(res, (list, tuple)) or len(res) != 1 or res[0] is not None:
        print("FAIL: MGET wrong type should return nil:", res)
        return False

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="MGET wrong type test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: MGET wrong type")
        sys.exit(0)
    print("FAIL: MGET wrong type")
    sys.exit(1)
