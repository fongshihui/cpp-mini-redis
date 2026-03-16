#include "../include/command_handler.h"

#include <cassert>
#include <vector>

namespace {

void expect_reply(const RedisReply& reply, RedisReplyType type, const std::string& value) {
    assert(reply.type == type);
    assert(reply.value == value);
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

    return 0;
}
