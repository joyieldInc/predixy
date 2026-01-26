#!/usr/bin/env python3
#
# Stress bulk parsing at buffer boundaries by splitting payload and CRLF.
#

import socket
from test_util import parse_args, make_client, exit_with_result


def read_line(sock):
    buf = bytearray()
    while True:
        ch = sock.recv(1)
        if not ch:
            raise ConnectionError("socket closed while reading line")
        buf += ch
        if buf.endswith(b"\r\n"):
            return bytes(buf[:-2])


def read_resp(sock):
    lead = sock.recv(1)
    if not lead:
        raise ConnectionError("socket closed before response")
    if lead == b"+":
        return read_line(sock)
    if lead == b"$":
        length = int(read_line(sock))
        if length < 0:
            return None
        data = b""
        while len(data) < length:
            chunk = sock.recv(length - len(data))
            if not chunk:
                raise ConnectionError("socket closed while reading bulk")
            data += chunk
        crlf = sock.recv(2)
        if crlf != b"\r\n":
            raise ValueError("invalid bulk terminator")
        return data
    raise ValueError(f"unexpected RESP lead byte: {lead!r}")


def is_predixy(client):
    try:
        info = client.info()
    except Exception:
        return False
    return info.get("redis_mode") == "proxy" or info.get("RedisMode") == "proxy"


def get_bufsize(client):
    try:
        res = client.execute_command("CONFIG", "GET", "BufSize")
    except Exception:
        return None
    if isinstance(res, (list, tuple)) and len(res) == 2:
        try:
            return int(res[1])
        except Exception:
            return None
    return None


def run_test(host, port):
    client = make_client(host, port)
    predixy = is_predixy(client)
    bufsize = get_bufsize(client) if predixy else None
    base_size = bufsize if bufsize and bufsize > 0 else 1024

    payload = b"a" * max(1, base_size // 2)
    prefix = f"*2\r\n$4\r\nping\r\n${len(payload)}\r\n".encode("ascii")
    chunk1 = prefix + payload

    # Use a fresh raw socket to control write boundaries.
    sock = socket.create_connection((host, port), timeout=2.0)
    try:
        sock.sendall(chunk1)
        sock.sendall(b"\r\n")
        resp = read_resp(sock)
        if resp != payload:
            print("FAIL: response mismatch length", len(resp or b""), "expected", len(payload))
            return False
    finally:
        sock.close()

    # Try an oversized bulk length to ensure the parser stays safe.
    if predixy:
        overflow = b"*2\r\n$4\r\nping\r\n$2147483648\r\nx\r\n"
        sock = socket.create_connection((host, port), timeout=2.0)
        try:
            sock.sendall(overflow)
            sock.close()
        except Exception as exc:
            print("WARN: overflow send failed:", exc)

    # Ensure server is still responsive.
    try:
        if client.ping() is not True:
            print("FAIL: ping after boundary request")
            return False
    except Exception as exc:
        print("FAIL: ping after boundary request:", exc)
        return False
    return True


if __name__ == "__main__":
    args = parse_args("Request parser boundary test")
    success = run_test(args.host, args.port)
    exit_with_result(success, "request parser boundary", "request parser boundary")
