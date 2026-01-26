#!/usr/bin/env python3
#
# Build and run the Buffer::fappend vsnprintf edge-case test.
#

import os
import subprocess
import tempfile
from test_util import parse_args, exit_with_result


def run_test(project_root):
    src = os.path.join(project_root, "test", "buffer_vsnprintf.cpp")
    if not os.path.exists(src):
        print("FAIL: buffer_vsnprintf.cpp not found")
        return False

    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "buffer_vsnprintf")
        cmd = [
            "g++",
            "-std=c++11",
            src,
            os.path.join(project_root, "src", "Buffer.cpp"),
            os.path.join(project_root, "src", "Alloc.cpp"),
            os.path.join(project_root, "src", "Logger.cpp"),
            os.path.join(project_root, "src", "LogFileSink.cpp"),
            os.path.join(project_root, "src", "Timer.cpp"),
            "-o",
            exe,
        ]
        try:
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception as exc:
            print("FAIL: compile buffer_vsnprintf:", exc)
            return False
        try:
            subprocess.check_call([exe], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError as exc:
            print("FAIL: buffer_vsnprintf returned", exc.returncode)
            return False
    return True


if __name__ == "__main__":
    _ = parse_args("Buffer vsnprintf test")
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    success = run_test(root)
    exit_with_result(success, "buffer vsnprintf", "buffer vsnprintf")
