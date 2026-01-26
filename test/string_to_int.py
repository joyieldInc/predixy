#!/usr/bin/env python3
#
# Build and run the String::toInt sign handling test.
#

import os
import subprocess
import tempfile
from test_util import parse_args, exit_with_result


def run_test(project_root):
    src = os.path.join(project_root, "test", "string_to_int.cpp")
    if not os.path.exists(src):
        print("FAIL: string_to_int.cpp not found")
        return False

    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "string_to_int")
        cmd = [
            "g++",
            "-std=c++11",
            src,
            "-o",
            exe,
        ]
        try:
            subprocess.check_call(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception as exc:
            print("FAIL: compile string_to_int:", exc)
            return False
        try:
            subprocess.check_call([exe], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError as exc:
            print("FAIL: string_to_int returned", exc.returncode)
            return False
    return True


if __name__ == "__main__":
    _ = parse_args("String toInt test")
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    success = run_test(root)
    exit_with_result(success, "string toInt", "string toInt")
