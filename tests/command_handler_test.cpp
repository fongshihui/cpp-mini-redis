#include "../include/command_handler.h"

#include <cassert>
#include <chrono>
#include <thread>
#include <vector>

namespace {

void expect_reply(const RedisReply& reply, RedisReplyType type, const std::string& value) {
    assert(reply.type == type);
    assert(reply.value == value);
}

void expect_array_size(const RedisReply& reply, size_t expected_size) {
    assert(reply.type == RedisReplyType::Array);
    assert(reply.elements.size() == expected_size);
}

}  // namespace

int main() {
    KVStore store;
    CommandHandler handler(store);

    expect_reply(handler.handle({"PING"}), RedisReplyType::SimpleString, "PONG");
    expect_reply(handler.handle({"PING", "hello from test"}), RedisReplyType::BulkString,
                 "hello from test");

    expect_reply(handler.handle({"SET", "name", "alice"}), RedisReplyType::SimpleString, "OK");
    expect_reply(handler.handle({"GET", "name"}), RedisReplyType::BulkString, "alice");
    expect_reply(handler.handle({"GET", "missing"}), RedisReplyType::NullBulkString, "");

    expect_reply(handler.handle({"EXISTS", "name"}), RedisReplyType::Integer, "1");
    expect_reply(handler.handle({"DEL", "name"}), RedisReplyType::Integer, "1");
    expect_reply(handler.handle({"EXISTS", "name"}), RedisReplyType::Integer, "0");

    expect_reply(handler.handle({"INCR", "counter"}), RedisReplyType::Integer, "1");
    expect_reply(handler.handle({"DECR", "counter"}), RedisReplyType::Integer, "0");

    handler.handle({"SET", "text", "not-a-number"});
    expect_reply(handler.handle({"INCR", "text"}), RedisReplyType::Error,
                 "ERR value is not an integer or out of range");
    expect_reply(handler.handle({"SET", "too", "many", "args"}), RedisReplyType::Error,
                 "ERR wrong number of arguments for 'set' command");

    expect_reply(handler.handle({"SET", "flash", "value", "EX", "1"}), RedisReplyType::SimpleString,
                 "OK");
    const RedisReply ttl_before_expiry = handler.handle({"TTL", "flash"});
    assert(ttl_before_expiry.type == RedisReplyType::Integer);
    assert(ttl_before_expiry.value == "1");
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    expect_reply(handler.handle({"GET", "flash"}), RedisReplyType::NullBulkString, "");
    expect_reply(handler.handle({"TTL", "flash"}), RedisReplyType::Integer, "-2");

    expect_reply(handler.handle({"SET", "profile", "alice"}), RedisReplyType::SimpleString, "OK");
    expect_reply(handler.handle({"GET", "profile"}), RedisReplyType::BulkString, "alice");
    expect_reply(handler.handle({"GET", "profile"}), RedisReplyType::BulkString, "alice");
    const RedisReply meta = handler.handle({"META", "profile"});
    expect_array_size(meta, 14);
    expect_reply(meta.elements[0], RedisReplyType::BulkString, "key");
    expect_reply(meta.elements[1], RedisReplyType::BulkString, "profile");
    expect_reply(meta.elements[10], RedisReplyType::BulkString, "access_count");
    assert(meta.elements[11].type == RedisReplyType::Integer);
    assert(std::stoi(meta.elements[11].value) >= 3);

    KVStore ranking_store;
    CommandHandler ranking_handler(ranking_store);
    ranking_handler.handle({"SET", "cold", "1"});
    ranking_handler.handle({"SET", "warm", "2"});
    ranking_handler.handle({"GET", "warm"});
    ranking_handler.handle({"GET", "warm"});
    const RedisReply top = ranking_handler.handle({"TOPKEYS", "2"});
    expect_array_size(top, 2);
    expect_reply(top.elements[0], RedisReplyType::BulkString, "warm:2");
    expect_reply(top.elements[1], RedisReplyType::BulkString, "cold:0");

    expect_reply(handler.handle({"SET", "badttl", "value", "PX", "10"}), RedisReplyType::Error,
                 "ERR syntax error");

    return 0;
}
