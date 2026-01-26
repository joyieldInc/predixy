#!/usr/bin/env python3
#
# Start a temporary predixy instance and verify SIGTERM shuts it down cleanly.
#

import os
import signal
import socket
import subprocess
import tempfile
import time

from test_util import parse_args, exit_with_result


def wait_for_port(host, port, timeout=5.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return True
        except Exception:
            time.sleep(0.05)
    return False


def start_predixy(root, redis_port):
    predixy_bin = os.path.join(root, "src", "predixy")
    if not os.path.exists(predixy_bin):
        raise RuntimeError("predixy binary not found")

    listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listen_sock.bind(("127.0.0.1", 0))
    listen_port = listen_sock.getsockname()[1]
    listen_sock.close()

    tmp_dir = tempfile.TemporaryDirectory()
    conf_path = os.path.join(tmp_dir.name, "predixy_test.conf")
    with open(conf_path, "w") as f:
        f.write(
            "Name PredixySignalTest\n"
            f"Bind 127.0.0.1:{listen_port}\n"
            "WorkerThreads 1\n"
            "ClientTimeout 3\n"
            "LogVerbSample 0\n"
            "LogDebugSample 0\n"
            "LogInfoSample 10000\n"
            "LogNoticeSample 1\n"
            "LogWarnSample 1\n"
            "LogErrorSample 1\n"
            "\n"
            "StandaloneServerPool {\n"
            "    RefreshMethod fixed\n"
            "    Group test {\n"
            f"        + 127.0.0.1:{redis_port}\n"
            "    }\n"
            "}\n"
        )

    proc = subprocess.Popen([predixy_bin, conf_path],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    if not wait_for_port("127.0.0.1", listen_port, timeout=5.0):
        proc.terminate()
        tmp_dir.cleanup()
        raise RuntimeError("predixy did not start")

    return proc, tmp_dir


def run_test(project_root, redis_port):
    proc, tmp_dir = start_predixy(project_root, redis_port)
    try:
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            print("FAIL: predixy did not exit after SIGTERM")
            proc.kill()
            return False
    finally:
        tmp_dir.cleanup()
    return True


if __name__ == "__main__":
    args = parse_args("Signal handling test", default_port=6380)
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    success = run_test(root, args.port)
    exit_with_result(success, "signal handling", "signal handling")
