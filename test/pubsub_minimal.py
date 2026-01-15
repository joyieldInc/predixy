#!/usr/bin/env python3
# 
# Minimal test to reproduce pubsub message queueing issue in Predixy
# Tests pass against Redis (6379) but fail against Predixy (7617)

import sys
from test_util import parse_args, make_clients, exit_with_result

def test_redis(host, port):
    """Test pubsub against Redis or Predixy"""
    print(f"\n=== Testing against {host}:{port} ===\n")
    
    c1, c2 = make_clients(host, port, count=2)
    
    ps = c1.pubsub()
    
    # Step 1: Subscribe to channel 'ch'
    print("1. Subscribing to channel 'ch'...")
    ps.subscribe('ch')
    
    # Step 2: Get subscribe confirmation
    print("2. Getting subscribe confirmation...")
    msg = ps.get_message(timeout=1.0)
    print(f"   Received: {msg}")
    if msg and msg.get('type') == 'subscribe' and msg.get('channel') in (b'ch', 'ch'):
        print("   ✓ Subscribe confirmation received")
    else:
        print(f"   ✗ Expected subscribe confirmation, got: {msg}")
        return False
    
    # Step 3: Publish a message
    print("3. Publishing 'hello' to channel 'ch'...")
    pub_result = c2.publish('ch', 'hello')
    print(f"   Publish returned: {pub_result}")
    
    # Step 4: Get the published message
    print("4. Getting published message...")
    msg = ps.get_message(timeout=1.0)
    print(f"   Received: {msg}")
    if msg and msg.get('type') == 'message':
        data = msg.get('data')
        if isinstance(data, bytes):
            data = data.decode('utf-8')
        if data == 'hello':
            print("   ✓ Published message received")
        else:
            print(f"   ✗ Expected 'hello', got: {data}")
            return False
    else:
        print(f"   ✗ Expected message, got: {msg}")
        return False
    
    # Step 5: Psubscribe to pattern 'ch*'
    print("5. Psubscribing to pattern 'ch*'...")
    ps.psubscribe('ch*')
    
    # Step 6: Get psubscribe confirmation (THIS FAILS WITH PREDIXY)
    print("6. Getting psubscribe confirmation...")
    msg = ps.get_message(timeout=1.0)
    print(f"   Received: {msg}")
    if msg and msg.get('type') == 'psubscribe':
        pattern = msg.get('channel') or msg.get('pattern')
        if isinstance(pattern, bytes):
            pattern = pattern.decode('utf-8')
        if pattern == 'ch*':
            print("   ✓ Psubscribe confirmation received")
            return True
        else:
            print(f"   ✗ Expected pattern 'ch*', got: {pattern}")
            return False
    else:
        print(f"   ✗ Expected psubscribe confirmation, got: {msg}")
        print(f"   This is the bug: old messages are returned instead of new ones")
        return False

if __name__ == '__main__':
    args = parse_args("Minimal pubsub test", require_port=True)
    host = args.host
    port = args.port
    
    try:
        success = test_redis(host, port)
        exit_with_result(success, "pubsub minimal", "pubsub minimal")
    except Exception as e:
        print(f"\n🔴 pubsub minimal error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
