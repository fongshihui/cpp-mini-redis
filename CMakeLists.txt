cmake_minimum_required(VERSION 3.10)

project(MiniRedis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_executable(
    mini_redis
    src/main.cpp
    src/server.cpp
    src/kvstore.cpp
    src/command_handler.cpp
)

enable_testing()

add_executable(
    command_handler_test
    tests/command_handler_test.cpp
    src/kvstore.cpp
    src/command_handler.cpp
)

add_test(NAME command_handler_test COMMAND command_handler_test)
