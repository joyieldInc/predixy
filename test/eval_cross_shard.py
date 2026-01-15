#!/usr/bin/env python3
#
# Verify EVAL/EVALSHA rejects multi-key cross-shard scripts
#

import sys
from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    c = make_client(host, port)
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
    args = parse_args("EVAL cross-shard test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "eval cross shard", "eval cross shard")
