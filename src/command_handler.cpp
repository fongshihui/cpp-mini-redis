#include "../include/command_handler.h"

#include <algorithm>
#include <cctype>

namespace {

std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

RedisReply wrong_arity(const std::string& command_name) {
    return {RedisReplyType::Error,
            "ERR wrong number of arguments for '" + command_name + "' command"};
}

}  // namespace

CommandHandler::CommandHandler(KVStore& kv) : store(kv) {}

RedisReply CommandHandler::handle(const std::vector<std::string>& args) {
    if (args.empty()) {
        return {RedisReplyType::Error, "ERR empty command"};
    }

    const std::string cmd = to_upper(args[0]);

    if (cmd == "PING") {
        if (args.size() > 2) {
            return wrong_arity("ping");
        }
        if (args.size() == 2) {
            return {RedisReplyType::BulkString, args[1]};
        }
        return {RedisReplyType::SimpleString, "PONG"};
    }

    if (cmd == "SET") {
        if (args.size() != 3) {
            return wrong_arity("set");
        }
        store.set(args[1], args[2]);
        return {RedisReplyType::SimpleString, "OK"};
    }

    if (cmd == "GET") {
        if (args.size() != 2) {
            return wrong_arity("get");
        }
        auto value = store.get(args[1]);
        if (!value.has_value()) {
            return {RedisReplyType::NullBulkString, ""};
        }
        return {RedisReplyType::BulkString, *value};
    }

    if (cmd == "DEL") {
        if (args.size() != 2) {
            return wrong_arity("del");
        }
        return {RedisReplyType::Integer, store.del(args[1]) ? "1" : "0"};
    }

    if (cmd == "EXISTS") {
        if (args.size() != 2) {
            return wrong_arity("exists");
        }
        return {RedisReplyType::Integer, store.exists(args[1]) ? "1" : "0"};
    }

    if (cmd == "INCR") {
        if (args.size() != 2) {
            return wrong_arity("incr");
        }
        try {
            return {RedisReplyType::Integer, std::to_string(store.incr(args[1]))};
        } catch (const std::exception&) {
            return {RedisReplyType::Error, "ERR value is not an integer or out of range"};
        }
    }

    if (cmd == "DECR") {
        if (args.size() != 2) {
            return wrong_arity("decr");
        }
        try {
            return {RedisReplyType::Integer, std::to_string(store.decr(args[1]))};
        } catch (const std::exception&) {
            return {RedisReplyType::Error, "ERR value is not an integer or out of range"};
        }
    }

    return {RedisReplyType::Error, "ERR unknown command '" + args[0] + "'"};
}
