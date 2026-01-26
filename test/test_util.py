#!/usr/bin/env python3
#
# Shared test utilities for argument parsing and Redis connections.
#

import argparse
import sys
import redis


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 7617


def add_host_port_args(parser, default_port=DEFAULT_PORT, require_port=False):
    parser.add_argument("-h", "--host", default=DEFAULT_HOST)
    parser.add_argument("-p", "--port", type=int, default=default_port,
                        required=require_port)
    return parser


def parse_args(description, default_port=DEFAULT_PORT, require_port=False):
    parser = argparse.ArgumentParser(conflict_handler="resolve",
                                     description=description)
    add_host_port_args(parser, default_port=default_port,
                       require_port=require_port)
    return parser.parse_args()


def get_host_port(args, host_attr="host", port_attr="port",
                  default_host=DEFAULT_HOST, default_port=DEFAULT_PORT):
    host = getattr(args, host_attr, None) or default_host
    port = getattr(args, port_attr, None) or default_port
    return host, port


def make_client(host, port):
    return redis.StrictRedis(host=host, port=port)


def make_clients(host, port, count=2):
    return [make_client(host, port) for _ in range(count)]


def report_result(success, pass_msg, fail_msg):
    if success:
        print(f"🟢 {pass_msg}")
        return True
    print(f"🔴 {fail_msg}")
    return False


def exit_with_result(success, pass_msg, fail_msg):
    report_result(success, pass_msg, fail_msg)
    sys.exit(0 if success else 1)
