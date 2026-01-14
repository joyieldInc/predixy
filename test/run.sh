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
PUBSUB_EXIT=0

uv run python3 test/basic.py || BASIC_EXIT=$?
uv run python3 test/pubsub.py || PUBSUB_EXIT=$?

TEST_EXIT=$((BASIC_EXIT + PUBSUB_EXIT))
exit $TEST_EXIT
