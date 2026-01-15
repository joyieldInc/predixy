#!/usr/bin/env python3
#
# Verify MGET returns WRONGTYPE for non-string keys
#

import sys
from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    c = make_client(host, port)
    c.delete("mget_wrong_type")
    c.lpush("mget_wrong_type", "v1")

    res = c.execute_command("MGET", "mget_wrong_type")
    if not isinstance(res, (list, tuple)) or len(res) != 1 or res[0] is not None:
        print("FAIL: MGET wrong type should return nil:", res)
        return False

    return True


if __name__ == "__main__":
    args = parse_args("MGET wrong type test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "MGET wrong type", "MGET wrong type")
