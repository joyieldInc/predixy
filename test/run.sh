#!/bin/bash
# Run predixy tests
# Starts fresh Redis and Predixy instances, runs tests, and stops them when done

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

# Check if redis-server is available
if ! command -v redis-server &> /dev/null; then
    echo "Error: 'redis-server' command not found"
    echo "Please install Redis"
    exit 1
fi

# Use fixed ports for test instances
TEST_REDIS_PORT=6380
TEST_PREDIXY_PORT=7618
TIMEOUT=10  # seconds

# Create temporary directory for test configs
TMP_DIR=$(mktemp -d)
trap "rm -rf '$TMP_DIR'" EXIT INT TERM

# Cleanup function
cleanup() {
    echo "Cleaning up test instances..."
    if [ -n "$REDIS_PID" ]; then
        kill $REDIS_PID 2>/dev/null || true
        wait $REDIS_PID 2>/dev/null || true
    fi
    if [ -n "$PREDIXY_PID" ]; then
        kill $PREDIXY_PID 2>/dev/null || true
        wait $PREDIXY_PID 2>/dev/null || true
    fi
    rm -rf "$TMP_DIR"
}

# Set up trap to ensure cleanup on exit
trap cleanup EXIT INT TERM

# Start fresh Redis instance
echo "Starting fresh Redis on port $TEST_REDIS_PORT..."
REDIS_PID=$(redis-server --port $TEST_REDIS_PORT --save "" --appendonly no > /dev/null 2>&1 & echo $!)
if [ -z "$REDIS_PID" ] || ! kill -0 $REDIS_PID 2>/dev/null; then
    echo "Error: Failed to start Redis"
    exit 1
fi

# Wait for Redis to be ready
echo "Waiting for Redis to start on port $TEST_REDIS_PORT..."
if ! uv run python3 "$SCRIPT_DIR/wait_for_port.py" localhost $TEST_REDIS_PORT $TIMEOUT; then
    echo "Error: Redis failed to start"
    exit 1
fi
echo "Redis is ready"

# Create temporary Predixy config pointing to test Redis
TEST_PREDIXY_CONFIG="$TMP_DIR/predixy_test.conf"
cat > "$TEST_PREDIXY_CONFIG" <<EOF
Name PredixyTest
Bind 0.0.0.0:$TEST_PREDIXY_PORT
WorkerThreads 1
ClientTimeout 300
LogVerbSample 0
LogDebugSample 0
LogInfoSample 10000
LogNoticeSample 1
LogWarnSample 1
LogErrorSample 1

Include $PROJECT_ROOT/conf/auth.conf

StandaloneServerPool {
    RefreshMethod fixed
    Group test {
        + 127.0.0.1:$TEST_REDIS_PORT
    }
}

Include $PROJECT_ROOT/conf/latency.conf
EOF

# Start fresh Predixy instance
echo "Starting fresh Predixy on port $TEST_PREDIXY_PORT..."
PREDIXY_PID=$("$PREDIXY_BIN" "$TEST_PREDIXY_CONFIG" > /dev/null 2>&1 & echo $!)

# Check if process died before waiting for port
if ! kill -0 $PREDIXY_PID 2>/dev/null; then
    echo "Error: predixy process died"
    exit 1
fi

# Wait for Predixy to start
echo "Waiting for Predixy to start on port $TEST_PREDIXY_PORT..."
if ! uv run python3 "$SCRIPT_DIR/wait_for_port.py" localhost $TEST_PREDIXY_PORT $TIMEOUT; then
    # Check if process died during wait
    if ! kill -0 $PREDIXY_PID 2>/dev/null; then
        echo "Error: predixy process died"
    fi
    exit 1
fi
echo "Predixy is ready"

# Run tests
echo "Running tests..."
cd "$PROJECT_ROOT"

TEST_EXIT=0

run_test() {
    local test_file=$1
    shift
    local port=$1
    shift
    uv run python3 "$test_file" -p "$port" "$@" || TEST_EXIT=$((TEST_EXIT + $?))
}

TESTS=(
    "test/basic.py"
    "test/pubsub_minimal.py"
    "test/pubsub.py"
    "test/pubsub_message_response.py"
    "test/pubsub_subscription_order.py"
    "test/pubsub_parser_reset.py"
    "test/null_response_handling.py"
    "test/request_parser_boundary.py"
    "test/request_parser_error_log.py"
    "test/response_parser_error_log.py"
    "test/pubsub_long_name.py"
    "test/pubsub_large_message.py"
    "test/transaction_forbid.py"
    "test/mget_wrong_type.py"
    "test/msetnx_atomicity.py"
    "test/eval_cross_shard.py"
)

run_tests_for_port() {
    local port=$1
    shift
    echo "Running tests against port $port..."
    for test_file in "${TESTS[@]}"; do
        run_test "$test_file" "$port"
    done
}

run_tests_for_port $TEST_REDIS_PORT
run_tests_for_port $TEST_PREDIXY_PORT

exit $TEST_EXIT
