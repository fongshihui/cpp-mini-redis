# C++ Mini Redis Server

A lightweight Redis-compatible in-memory key-value store implementation in C++. This project provides a basic Redis server that supports core Redis commands and can be used for learning, testing, and development purposes.

## Features

- ✅ **Redis Protocol (RESP) Support** - Parses RESP array requests and emits RESP-compatible replies
- ✅ **Basic Commands** - PING, SET, GET, DEL, EXISTS, INCR, DECR
- ✅ **In-Memory Storage** - Fast key-value storage with string support
- ✅ **TCP Server** - Listens on port 6379 (standard Redis port)
- ✅ **Thread-Safe** - Handles multiple client connections
- ✅ **Cross-Platform** - Works on macOS, Linux, and Windows

## Quick Start

### Building the Server

```bash
# Create build directory
mkdir build
cd build

# Generate build files
cmake ..

# Compile the project
make
```

### Running the Server

```bash
# From the build directory
./mini_redis

# Server will start and display:
# Server started on port 6379
```

### Testing with Redis CLI

If you have `redis-cli` installed:
```bash
redis-cli
127.0.0.1:6379> SET test_key "hello world"
127.0.0.1:6379> GET test_key
"hello world"
127.0.0.1:6379> PING
PONG
```

### Testing with Python Client

A comprehensive Python client is included for testing without `redis-cli`:

```bash
# Run the Python client test suite
python3 redis_client.py

# Expected output:
# Connected to Redis server at localhost:6379
# === Testing Redis Client ===
# 1. Testing PING:
#    Response: PONG
# 2. Testing SET and GET:
#    SET result: OK
#    GET result: hello_world
# ... and more tests
```

## Python Client Usage

The included `redis_client.py` provides a full-featured Redis client that demonstrates how real applications interact with Redis servers:

```python
from redis_client import RedisClient

# Create and connect to server
client = RedisClient(host='localhost', port=6379)
client.connect()

# Basic operations
client.set("user:1:name", "Alice")
client.set("user:1:email", "alice@example.com")

name = client.get("user:1:name")  # Returns "Alice"
email = client.get("user:1:email")  # Returns "alice@example.com"

# Numeric operations
client.set("counter", "100")
client.incr("counter")  # Returns 101
client.decr("counter")  # Returns 100

# Key management
client.exists("counter")  # Returns 1 (true)
client.delete("counter")  # Returns 1 (success)
client.exists("counter")  # Returns 0 (false)

# Clean up
client.disconnect()
```

## Supported Commands

| Command | Description | Example |
|---------|-------------|---------|
| `PING` | Test server connectivity | `PING` → `PONG`, `PING hello` → `hello` |
| `SET` | Store a key-value pair | `SET name John` → `OK` |
| `GET` | Retrieve a value by key | `GET name` → `John` |
| `DEL` | Delete a key | `DEL name` → `1` |
| `EXISTS` | Check if key exists | `EXISTS name` → `1` or `0` |
| `INCR` | Increment numeric value | `INCR counter` → `11` |
| `DECR` | Decrement numeric value | `DECR counter` → `10` |

## Project Structure

```
cpp-mini-redis/
├── include/           # Header files
│   ├── command_handler.h
│   ├── kvstore.h
│   └── server.h
├── src/               # Source files
│   ├── command_handler.cpp
│   ├── kvstore.cpp
│   ├── main.cpp
│   └── server.cpp
├── build/             # Build directory (created)
├── redis_client.py    # Python test client
├── CMakeLists.txt     # Build configuration
└── README.md         # This file
```

## Key Components

- **`KVStore`**: In-memory key-value storage engine
- **`CommandHandler`**: Validates command arguments and returns typed Redis replies
- **`Server`**: TCP server that handles client connections
- **`redis_client.py`**: Python client for testing and demonstration

## Development

### Adding New Commands

1. Add command handling in `command_handler.cpp`
2. Implement the command logic in the appropriate class
3. Ensure `server.cpp` can decode and encode the RESP shape for the new command
4. Update the Python client to support the new command
5. Run `ctest --output-on-failure`

### Building and Testing

```bash
# Clean build
rm -rf build/
mkdir build && cd build
cmake .. && make

# Run the native regression tests
ctest --output-on-failure

# Run server
./mini_redis

# In another terminal, test with Python client
python3 ../redis_client.py
```

## Limitations

- One thread per client connection rather than an event-driven architecture
- Basic in-memory storage (no persistence)
- Limited command set compared to full Redis
- No authentication or security features

## Use Cases

- Learning Redis protocol implementation
- Testing Redis client applications
- Educational purposes for C++ network programming
- Lightweight development and testing environment

## License

This project is open source and available under the MIT License.
