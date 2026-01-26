#!/usr/bin/env python3
#
# Start a temporary predixy instance with a fake backend that returns
# invalid responses to exercise response parser error logging safely.
#

import os
import socket
import subprocess
import tempfile
import threading
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


def start_fake_backend(host="127.0.0.1"):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, 0))
    server.listen(1)
    port = server.getsockname()[1]

    def handler():
        try:
            conn, _ = server.accept()
            conn.recv(1024)
            conn.sendall(b"!")
            conn.close()
        finally:
            server.close()

    thread = threading.Thread(target=handler, daemon=True)
    thread.start()
    return port


def start_predixy(root, backend_port):
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
            "Name PredixyRespParserTest\n"
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
            f"        + 127.0.0.1:{backend_port}\n"
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

    return proc, listen_port, tmp_dir


def run_test(project_root):
    backend_port = start_fake_backend()
    proc, predixy_port, tmp_dir = start_predixy(project_root, backend_port)
    try:
        sock = socket.create_connection(("127.0.0.1", predixy_port), timeout=1.0)
        sock.sendall(b"*1\r\n$4\r\nping\r\n")
        try:
            sock.recv(16)
        except Exception:
            pass
        sock.close()

        time.sleep(0.2)
        if proc.poll() is not None:
            print("FAIL: predixy exited after invalid backend response")
            return False

        try:
            with socket.create_connection(("127.0.0.1", predixy_port), timeout=1.0):
                pass
        except Exception as exc:
            print("FAIL: predixy not accepting connections:", exc)
            return False
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2.0)
        except Exception:
            proc.kill()
        tmp_dir.cleanup()

    return True


if __name__ == "__main__":
    args = parse_args("Response parser error logging test")
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    success = run_test(root)
    exit_with_result(success, "response parser error logging",
                     "response parser error logging")
