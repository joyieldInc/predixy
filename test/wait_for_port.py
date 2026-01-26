#!/usr/bin/env python3
"""
Wait for a port to become available.
Exits with code 0 when the port is listening, or 1 if timeout is reached.
"""
import socket
import sys
import time

def is_port_listening(host, port):
    """Check if a port is listening on the given host."""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(0.1)
        result = sock.connect_ex((host, port))
        sock.close()
        return result == 0
    except Exception:
        return False

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <host> <port> [timeout]", file=sys.stderr)
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2])
    timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 10.0
    sleep_interval = 0.5
    
    elapsed = 0.0
    while elapsed < timeout:
        if is_port_listening(host, port):
            sys.exit(0)
        time.sleep(sleep_interval)
        elapsed += sleep_interval
    
    # Timeout reached
    print(f"Error: Port {port} on {host} did not become available within {timeout} seconds", file=sys.stderr)
    sys.exit(1)

if __name__ == "__main__":
    main()
