#!/usr/bin/env python3
# 
# Minimal test to reproduce pubsub message queueing issue in Predixy
# Tests pass against Redis (6379) but fail against Predixy (7617)

import redis
import sys
import argparse

def test_redis(host, port):
    """Test pubsub against Redis or Predixy"""
    print(f"\n=== Testing against {host}:{port} ===\n")
    
    c1 = redis.StrictRedis(host=host, port=port)
    c2 = redis.StrictRedis(host=host, port=port)
    
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
    parser = argparse.ArgumentParser(description='Minimal pubsub test')
    parser.add_argument('--host', default='127.0.0.1', help='Host (default: 127.0.0.1)')
    parser.add_argument('-p', '--port', type=int, required=True, help='Port (6379 for Redis, 7617 for Predixy)')
    args = parser.parse_args()
    
    host = args.host
    port = args.port
    
    try:
        success = test_redis(host, port)
        if success:
            print("\n✓ Test PASSED")
            sys.exit(0)
        else:
            print("\n✗ Test FAILED")
            sys.exit(1)
    except Exception as e:
        print(f"\n✗ Test ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
