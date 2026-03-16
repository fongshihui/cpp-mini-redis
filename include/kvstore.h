#pragma once

#include <cctype>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_map>

struct KeyMetadata {
    std::string key;
    size_t value_size;
    std::int64_t created_at_ms;
    std::int64_t updated_at_ms;
    std::int64_t last_accessed_at_ms;
    std::uint64_t access_count;
    std::int64_t ttl_ms;
};

class KVStore {
private:
    struct Entry {
        std::string value;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point updated_at;
        std::chrono::steady_clock::time_point last_accessed_at;
        std::optional<std::chrono::steady_clock::time_point> expires_at;
        std::uint64_t access_count = 0;
    };

    std::unordered_map<std::string, Entry> entries;
    std::mutex mtx;

    using Clock = std::chrono::steady_clock;

    bool isInteger(const std::string& s) const {
        if (s.empty()) return false;

        size_t start = 0;
        if (s[0] == '-') start = 1;
        if (start == s.length()) return false;

        for (size_t i = start; i < s.length(); i++) {
            if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        }
        return true;
    }

    int stringToInt(const std::string& s) const {
        try {
            return std::stoi(s);
        } catch (const std::exception&) {
            throw std::runtime_error("value is not an integer");
        }
    }

    static std::int64_t toUnixMillis(const Clock::time_point& time_point);
    static std::int64_t ttlMillis(
        const std::optional<Clock::time_point>& expires_at,
        const Clock::time_point& now
    );
    bool eraseIfExpired(const std::string& key, const Clock::time_point& now);
    void touchForRead(Entry& entry, const Clock::time_point& now);

public:
    void set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    int incr(const std::string& key);
    int decr(const std::string& key);
    bool expire(const std::string& key, int ttl_seconds);
    int ttl(const std::string& key);
    std::optional<KeyMetadata> metadata(const std::string& key);
    std::vector<std::pair<std::string, std::uint64_t>> topKeys(size_t limit);
};
