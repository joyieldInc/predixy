#!/bin/bash
# Run predixy tests
# Starts predixy, runs tests, and stops predixy when done

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PREDIXY_BIN="$PROJECT_ROOT/src/predixy"
CONFIG_FILE="$PROJECT_ROOT/conf/predixy.conf"

# Check if uv is available
if ! command -v uv &> /dev/null; then
    echo "Error: 'uv' command not found"
    echo "Please install uv: https://github.com/astral-sh/uv"
    exit 1
fi

# Check if predixy binary exists
if [ ! -f "$PREDIXY_BIN" ]; then
    echo "Error: predixy binary not found at $PREDIXY_BIN"
    echo "Please build predixy first with 'make'"
    exit 1
fi

# Start predixy in the background
echo "Starting predixy..."
PREDIXY_PID=$("$PREDIXY_BIN" "$CONFIG_FILE" > /dev/null 2>&1 & echo $!)

# Set up trap to ensure predixy is stopped on exit
trap "echo 'Stopping predixy...'; kill $PREDIXY_PID 2>/dev/null || true; wait $PREDIXY_PID 2>/dev/null || true" EXIT INT TERM

# Wait for predixy to start (check if port is listening)
PREDIXY_PORT=7617
TIMEOUT=10  # seconds
echo "Waiting for predixy to start on port $PREDIXY_PORT..."

# Check if process died before waiting for port
if ! kill -0 $PREDIXY_PID 2>/dev/null; then
    echo "Error: predixy process died"
    exit 1
fi

# Wait for port to become available
if uv run python3 "$SCRIPT_DIR/wait_for_port.py" localhost $PREDIXY_PORT $TIMEOUT; then
    echo "predixy is ready"
else
    # Check if process died during wait
    if ! kill -0 $PREDIXY_PID 2>/dev/null; then
        echo "Error: predixy process died"
    fi
    exit 1
fi

# Run tests
echo "Running tests..."
cd "$PROJECT_ROOT"

BASIC_EXIT=0
PUBSUB_REDIS_EXIT=0
PUBSUB_MINIMAL_EXIT=0
PUBSUB_EXIT=0
PUBSUB_MESSAGE_EXIT=0
PUBSUB_ORDER_EXIT=0
PUBSUB_RESET_EXIT=0
NULL_RESPONSE_EXIT=0
PUBSUB_LONG_EXIT=0
TRANSACTION_FORBID_EXIT=0
MGET_WRONG_TYPE_EXIT=0
MSETNX_ATOMICITY_EXIT=0
EVAL_CROSS_SHARD_EXIT=0

uv run python3 test/basic.py || BASIC_EXIT=$?
uv run python3 test/pubsub_minimal.py -p 7617 || PUBSUB_REDIS_EXIT=$?
uv run python3 test/pubsub_minimal.py -p 6379 || PUBSUB_MINIMAL_EXIT=$?
uv run python3 test/pubsub.py || PUBSUB_EXIT=$?
uv run python3 test/pubsub_message_response.py -p 7617 || PUBSUB_MESSAGE_EXIT=$?
uv run python3 test/pubsub_subscription_order.py -p 7617 || PUBSUB_ORDER_EXIT=$?
uv run python3 test/pubsub_parser_reset.py -p 7617 || PUBSUB_RESET_EXIT=$?
uv run python3 test/null_response_handling.py -p 7617 || NULL_RESPONSE_EXIT=$?
uv run python3 test/pubsub_long_name.py -p 7617 || PUBSUB_LONG_EXIT=$?
uv run python3 test/transaction_forbid.py -p 7617 || TRANSACTION_FORBID_EXIT=$?
uv run python3 test/mget_wrong_type.py -p 7617 || MGET_WRONG_TYPE_EXIT=$?
uv run python3 test/msetnx_atomicity.py -p 7617 || MSETNX_ATOMICITY_EXIT=$?
uv run python3 test/eval_cross_shard.py -p 7617 || EVAL_CROSS_SHARD_EXIT=$?

TEST_EXIT=$((BASIC_EXIT + PUBSUB_REDIS_EXIT + PUBSUB_MINIMAL_EXIT + PUBSUB_EXIT + PUBSUB_MESSAGE_EXIT + PUBSUB_ORDER_EXIT + PUBSUB_RESET_EXIT + NULL_RESPONSE_EXIT + PUBSUB_LONG_EXIT + TRANSACTION_FORBID_EXIT + MGET_WRONG_TYPE_EXIT + MSETNX_ATOMICITY_EXIT + EVAL_CROSS_SHARD_EXIT))
exit $TEST_EXIT
