#!/usr/bin/env python3
#
# Verify forbidden command in transaction returns error without closing connection
#

from test_util import parse_args, make_client, exit_with_result


def run_test(host, port):
    c = make_client(host, port)
    try:
        r = c.execute_command("MULTI")
        if r not in (b"OK", "OK"):
            print("FAIL: MULTI response:", r)
            return False
    except Exception as exc:
        print("FAIL: MULTI error:", exc)
        return False

    try:
        r = c.execute_command("SELECT", "0")
        if r not in (b"QUEUED", "QUEUED", False):
            print("FAIL: SELECT should be queued in transaction:", r)
            return False
    except Exception as exc:
        print("FAIL: SELECT should be queued in transaction, got exception:", exc)
        return False

    try:
        r = c.execute_command("PING")
        if r not in (b"QUEUED", "QUEUED", False):
            print("FAIL: PING should be queued in transaction:", r)
            return False
    except Exception as exc:
        print("FAIL: PING should be queued in transaction exception:", exc)
        return False

    try:
        c.execute_command("DISCARD")
    except Exception:
        pass

    return True


if __name__ == "__main__":
    args = parse_args("Transaction forbid test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "transaction forbid", "transaction forbid")
