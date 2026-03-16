#include "../include/command_handler.h"

#include <algorithm>
#include <cctype>
#include <optional>

namespace {

std::string to_upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

RedisReply wrong_arity(const std::string& command_name) {
    return {RedisReplyType::Error,
            "ERR wrong number of arguments for '" + command_name + "' command",
            {}};
}

RedisReply bulk(const std::string& value) {
    return {RedisReplyType::BulkString, value, {}};
}

RedisReply integer(long long value) {
    return {RedisReplyType::Integer, std::to_string(value), {}};
}

RedisReply error(const std::string& value) {
    return {RedisReplyType::Error, value, {}};
}

RedisReply simple(const std::string& value) {
    return {RedisReplyType::SimpleString, value, {}};
}

std::optional<int> parse_positive_int(const std::string& value) {
    try {
        size_t parsed = 0;
        int number = std::stoi(value, &parsed);
        if (parsed != value.size()) {
            return std::nullopt;
        }
        if (number <= 0) {
            return std::nullopt;
        }
        return number;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

RedisReply metadataReply(const KeyMetadata& metadata) {
    return {
        RedisReplyType::Array,
        "",
        {
            bulk("key"),
            bulk(metadata.key),
            bulk("value_size"),
            integer(static_cast<long long>(metadata.value_size)),
            bulk("created_at_ms"),
            integer(metadata.created_at_ms),
            bulk("updated_at_ms"),
            integer(metadata.updated_at_ms),
            bulk("last_accessed_at_ms"),
            integer(metadata.last_accessed_at_ms),
            bulk("access_count"),
            integer(static_cast<long long>(metadata.access_count)),
            bulk("ttl_ms"),
            integer(metadata.ttl_ms),
        },
    };
}

RedisReply topKeysReply(const std::vector<std::pair<std::string, std::uint64_t>>& ranked_keys) {
    std::vector<RedisReply> elements;
    elements.reserve(ranked_keys.size());
    for (const auto& [key, hits] : ranked_keys) {
        elements.push_back(bulk(key + ":" + std::to_string(hits)));
    }

    return {RedisReplyType::Array, "", std::move(elements)};
}

}  // namespace

CommandHandler::CommandHandler(KVStore& kv) : store(kv) {}

RedisReply CommandHandler::handle(const std::vector<std::string>& args) {
    if (args.empty()) {
        return error("ERR empty command");
    }

    const std::string cmd = to_upper(args[0]);

    if (cmd == "PING") {
        if (args.size() > 2) {
            return wrong_arity("ping");
        }
        if (args.size() == 2) {
            return bulk(args[1]);
        }
        return simple("PONG");
    }

    if (cmd == "SET") {
        if (args.size() != 3 && args.size() != 5) {
            return wrong_arity("set");
        }

        std::optional<int> ttl_seconds = std::nullopt;
        if (args.size() == 5) {
            if (to_upper(args[3]) != "EX") {
                return error("ERR syntax error");
            }
            ttl_seconds = parse_positive_int(args[4]);
            if (!ttl_seconds.has_value()) {
                return error("ERR invalid expire time in 'set' command");
            }
        }

        store.set(args[1], args[2], ttl_seconds);
        return simple("OK");
    }

    if (cmd == "GET") {
        if (args.size() != 2) {
            return wrong_arity("get");
        }
        auto value = store.get(args[1]);
        if (!value.has_value()) {
            return {RedisReplyType::NullBulkString, "", {}};
        }
        return bulk(*value);
    }

    if (cmd == "DEL") {
        if (args.size() != 2) {
            return wrong_arity("del");
        }
        return integer(store.del(args[1]) ? 1 : 0);
    }

    if (cmd == "EXISTS") {
        if (args.size() != 2) {
            return wrong_arity("exists");
        }
        return integer(store.exists(args[1]) ? 1 : 0);
    }

    if (cmd == "INCR") {
        if (args.size() != 2) {
            return wrong_arity("incr");
        }
        try {
            return integer(store.incr(args[1]));
        } catch (const std::exception&) {
            return error("ERR value is not an integer or out of range");
        }
    }

    if (cmd == "DECR") {
        if (args.size() != 2) {
            return wrong_arity("decr");
        }
        try {
            return integer(store.decr(args[1]));
        } catch (const std::exception&) {
            return error("ERR value is not an integer or out of range");
        }
    }

    if (cmd == "EXPIRE") {
        if (args.size() != 3) {
            return wrong_arity("expire");
        }

        const auto ttl_seconds = parse_positive_int(args[2]);
        if (!ttl_seconds.has_value()) {
            return error("ERR value is not an integer or out of range");
        }

        return integer(store.expire(args[1], ttl_seconds.value()) ? 1 : 0);
    }

    if (cmd == "TTL") {
        if (args.size() != 2) {
            return wrong_arity("ttl");
        }
        return integer(store.ttl(args[1]));
    }

    if (cmd == "META") {
        if (args.size() != 2) {
            return wrong_arity("meta");
        }

        const auto metadata = store.metadata(args[1]);
        if (!metadata.has_value()) {
            return {RedisReplyType::NullBulkString, "", {}};
        }

        return metadataReply(metadata.value());
    }

    if (cmd == "TOPKEYS") {
        if (args.size() != 2) {
            return wrong_arity("topkeys");
        }

        const auto limit = parse_positive_int(args[1]);
        if (!limit.has_value()) {
            return error("ERR value is not an integer or out of range");
        }

        return topKeysReply(store.topKeys(static_cast<size_t>(limit.value())));
    }

    return error("ERR unknown command '" + args[0] + "'");
}
