#!/usr/bin/env python3
#
# Build and run the Handler::mStop atomic type check.
#

import os
import platform
import subprocess
import tempfile
from test_util import parse_args, exit_with_result


def run_test(project_root):
    src = os.path.join(project_root, "test", "handler_stop_atomic.cpp")
    if not os.path.exists(src):
        print("FAIL: handler_stop_atomic.cpp not found")
        return False

    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "handler_stop_atomic")
    cmd = [
            "g++",
            "-std=c++11",
            src,
            "-o",
            exe,
        ]
    sysname = platform.system()
    if sysname == "Darwin":
        cmd.insert(1, "-D_KQUEUE_")
    elif sysname == "Linux":
        cmd.insert(1, "-D_EPOLL_")
        try:
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception as exc:
            print("FAIL: compile handler_stop_atomic:", exc)
            return False
        try:
            subprocess.check_call([exe], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError as exc:
            print("FAIL: handler_stop_atomic returned", exc.returncode)
            return False
    return True


if __name__ == "__main__":
    _ = parse_args("Handler stop atomic test")
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    success = run_test(root)
    exit_with_result(success, "handler stop atomic", "handler stop atomic")
