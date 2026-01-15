#!/usr/bin/env python3
#
# Verify MSETNX does not partially apply changes when it returns 0
#

import sys
from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    c = make_client(host, port)
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
    args = parse_args("MSETNX atomicity test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "MSETNX atomicity", "MSETNX atomicity")
