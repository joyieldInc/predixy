#!/usr/bin/env python3
#
# Exercise INFO while other commands are running to catch race regressions.
#

import threading
import time
import redis
from test_util import parse_args, get_host_port, exit_with_result


def run_load(client, stop_event, errors):
    i = 0
    while not stop_event.is_set():
        try:
            key = "info_concurrent:%d" % i
            client.set(key, "v")
            client.get(key)
            i += 1
        except Exception as exc:
            errors.append(("load", str(exc)))
            return


def run_info(client, stop_event, errors):
    while not stop_event.is_set():
        try:
            client.info()
        except Exception as exc:
            errors.append(("info", str(exc)))
            return


def run_test(host, port):
    client = redis.StrictRedis(host=host, port=port)
    stop_event = threading.Event()
    errors = []

    threads = [
        threading.Thread(target=run_load, args=(client, stop_event, errors)),
        threading.Thread(target=run_load, args=(client, stop_event, errors)),
        threading.Thread(target=run_info, args=(client, stop_event, errors)),
    ]
    for t in threads:
        t.start()

    time.sleep(1.5)
    stop_event.set()
    for t in threads:
        t.join()

    if errors:
        print("FAIL: concurrent INFO errors", errors[0])
        return False
    return True


if __name__ == "__main__":
    args = parse_args("INFO concurrent test")
    host, port = get_host_port(args)
    success = run_test(host, port)
    exit_with_result(success, "info concurrent", "info concurrent")
