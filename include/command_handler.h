#pragma once

#include <string>
#include <vector>

#include "kvstore.h"

enum class RedisReplyType {
    SimpleString,
    BulkString,
    Integer,
    Error,
    NullBulkString,
    Array,
};

struct RedisReply {
    RedisReplyType type;
    std::string value;
    std::vector<RedisReply> elements;
};

class CommandHandler {
private:
    KVStore& store;

public:
    CommandHandler(KVStore& kv);
    RedisReply handle(const std::vector<std::string>& args);
};
