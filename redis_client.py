#!/usr/bin/env python3
"""
Simple Redis client for testing the mini Redis server.
This client implements basic Redis protocol (RESP) to communicate with the server.
"""

import socket
import sys

class RedisClient:
    def __init__(self, host='localhost', port=6379):
        self.host = host
        self.port = port
        self.socket = None
        
    def connect(self):
        """Establish connection to Redis server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            print(f"Connected to Redis server at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def disconnect(self):
        """Close connection"""
        if self.socket:
            self.socket.close()
            self.socket = None
            print("Disconnected from Redis server")
    
    def _send_command(self, *parts):
        """Send a Redis command using RESP protocol."""
        if not self.socket:
            raise ConnectionError("Not connected to server")

        if len(parts) == 1 and isinstance(parts[0], (list, tuple)):
            parts = tuple(parts[0])

        parts = [str(part) for part in parts]

        # Convert command to RESP format while preserving spaces inside args.
        resp_command = f"*{len(parts)}\r\n"
        for part in parts:
            resp_command += f"${len(part)}\r\n{part}\r\n"
        
        # Send command
        self.socket.send(resp_command.encode('utf-8'))
        
        # Read response
        response = self._read_response()
        return response
    
    def _read_response(self):
        """Read and parse RESP response"""
        line = self._read_line()
        if not line:
            return None
            
        first_char = line[0]
        
        if first_char == '+':  # Simple string
            return line[1:]
        elif first_char == '-':  # Error
            raise Exception(f"Redis error: {line[1:]}")
        elif first_char == ':':  # Integer
            return int(line[1:])
        elif first_char == '$':  # Bulk string
            length = int(line[1:])
            if length == -1:  # Null bulk string
                return None
            data = self._read_bytes(length)
            self._read_line()  # Read trailing \r\n
            return data.decode('utf-8')
        elif first_char == '*':  # Array
            count = int(line[1:])
            if count == -1:  # Null array
                return None
            return [self._read_response() for _ in range(count)]
        else:
            raise Exception(f"Unknown response type: {first_char}")
    
    def _read_line(self):
        """Read until \r\n"""
        buffer = b''
        while True:
            char = self.socket.recv(1)
            if char == b'':
                return None
            buffer += char
            if buffer.endswith(b'\r\n'):
                return buffer[:-2].decode('utf-8')
    
    def _read_bytes(self, length):
        """Read exactly length bytes"""
        data = b''
        while len(data) < length:
            chunk = self.socket.recv(length - len(data))
            if not chunk:
                raise ConnectionError("Connection closed")
            data += chunk
        return data
    
    # Redis command methods
    def ping(self, message=None):
        """PING command"""
        if message is None:
            return self._send_command("PING")
        return self._send_command("PING", message)
    
    def set(self, key, value):
        """SET command"""
        return self._send_command("SET", key, value)
    
    def get(self, key):
        """GET command"""
        return self._send_command("GET", key)
    
    def delete(self, key):
        """DEL command"""
        return self._send_command("DEL", key)
    
    def exists(self, key):
        """EXISTS command"""
        return self._send_command("EXISTS", key)
    
    def incr(self, key):
        """INCR command"""
        return self._send_command("INCR", key)
    
    def decr(self, key):
        """DECR command"""
        return self._send_command("DECR", key)

def main():
    """Example usage of the Redis client"""
    client = RedisClient()
    
    try:
        # Connect to server
        if not client.connect():
            return
        
        print("\n=== Testing Redis Client ===")
        
        # Test PING
        print("1. Testing PING:")
        result = client.ping()
        print(f"   Response: {result}")
        
        # Test SET and GET
        print("\n2. Testing SET and GET:")
        set_result = client.set("test_key", "hello_world")
        print(f"   SET result: {set_result}")
        
        get_result = client.get("test_key")
        print(f"   GET result: {get_result}")
        
        # Test numeric operations
        print("\n3. Testing numeric operations:")
        client.set("counter", "10")
        print(f"   Initial counter: {client.get('counter')}")
        
        incr_result = client.incr("counter")
        print(f"   After INCR: {incr_result}")
        
        decr_result = client.decr("counter")
        print(f"   After DECR: {decr_result}")
        
        # Test EXISTS and DELETE
        print("\n4. Testing EXISTS and DELETE:")
        exists_before = client.exists("test_key")
        print(f"   test_key exists before delete: {exists_before}")
        
        delete_result = client.delete("test_key")
        print(f"   DELETE result: {delete_result}")
        
        exists_after = client.exists("test_key")
        print(f"   test_key exists after delete: {exists_after}")
        
        # Test with message
        print("\n5. Testing PING with message:")
        ping_msg = client.ping("hello from python")
        print(f"   PING with message: {ping_msg}")
        
        print("\n=== All tests completed successfully! ===")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main()
