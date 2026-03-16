cmake_minimum_required(VERSION 3.10)

project(MiniRedis)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(
    mini_redis
    src/main.cpp
    src/server.cpp
    src/kvstore.cpp
    src/command_handler.cpp
)