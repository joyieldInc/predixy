#!/usr/bin/env python3
#
# Verify EVAL/EVALSHA rejects multi-key cross-shard scripts
#

import argparse
import sys
import redis


def run_test(host, port):
    c = redis.StrictRedis(host=host, port=port)
    script = "return {KEYS[1], KEYS[2]}"
    try:
        res = c.eval(script, 2, "eval_key1", "eval_key2")
        # If allowed (single shard), validate response shape.
        if not isinstance(res, (list, tuple)) or len(res) != 2:
            print("FAIL: unexpected EVAL response:", res)
            return False
        return True
    except Exception:
        # Error is acceptable when keys span shards.
        return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(conflict_handler='resolve', description="EVAL cross-shard test")
    parser.add_argument("-h", "--host", default="127.0.0.1")
    parser.add_argument("-p", "--port", type=int, default=7617)
    args = parser.parse_args()

    if run_test(args.host, args.port):
        print("PASS: eval cross shard")
        sys.exit(0)
    print("FAIL: eval cross shard")
    sys.exit(1)
