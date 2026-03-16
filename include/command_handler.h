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
};

struct RedisReply {
    RedisReplyType type;
    std::string value;
};

class CommandHandler {
private:
    KVStore& store;

public:
    CommandHandler(KVStore& kv);
    RedisReply handle(const std::vector<std::string>& args);
};
