#include "../include/kvstore.h"

#include <algorithm>
#include <limits>

namespace {

using Clock = std::chrono::steady_clock;

std::chrono::system_clock::time_point approximateSystemTime(const Clock::time_point& time_point) {
    const auto now_steady = Clock::now();
    const auto now_system = std::chrono::system_clock::now();
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        now_system + (time_point - now_steady)
    );
}

}  // namespace

std::int64_t KVStore::toUnixMillis(const Clock::time_point& time_point) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               approximateSystemTime(time_point).time_since_epoch()
    )
        .count();
}

std::int64_t KVStore::ttlMillis(
    const std::optional<Clock::time_point>& expires_at,
    const Clock::time_point& now
) {
    if (!expires_at.has_value()) {
        return -1;
    }

    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(*expires_at - now).count();
    return remaining > 0 ? remaining : 0;
}

bool KVStore::eraseIfExpired(const std::string& key, const Clock::time_point& now) {
    const auto it = entries.find(key);
    if (it == entries.end()) {
        return false;
    }

    if (it->second.expires_at.has_value() && it->second.expires_at.value() <= now) {
        entries.erase(it);
        return true;
    }

    return false;
}

void KVStore::touchForRead(Entry& entry, const Clock::time_point& now) {
    entry.last_accessed_at = now;
    ++entry.access_count;
}

void KVStore::set(const std::string& key, const std::string& value, std::optional<int> ttl_seconds) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();

    auto it = entries.find(key);
    if (it == entries.end()) {
        Entry entry;
        entry.value = value;
        entry.created_at = now;
        entry.updated_at = now;
        entry.last_accessed_at = now;
        entry.expires_at = ttl_seconds.has_value()
                               ? std::optional<Clock::time_point>(
                                     now + std::chrono::seconds(ttl_seconds.value())
                                 )
                               : std::nullopt;
        entries[key] = std::move(entry);
        return;
    }

    it->second.value = value;
    it->second.updated_at = now;
    it->second.last_accessed_at = now;
    it->second.expires_at = ttl_seconds.has_value()
                                ? std::optional<Clock::time_point>(
                                      now + std::chrono::seconds(ttl_seconds.value())
                                  )
                                : std::nullopt;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();

    if (eraseIfExpired(key, now)) {
        return std::nullopt;
    }

    auto it = entries.find(key);
    if (it == entries.end()) {
        return std::nullopt;
    }

    touchForRead(it->second, now);
    return it->second.value;
}

bool KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    eraseIfExpired(key, Clock::now());
    return entries.erase(key) > 0;
}

bool KVStore::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();
    if (eraseIfExpired(key, now)) {
        return false;
    }

    return entries.find(key) != entries.end();
}

int KVStore::incr(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();
    eraseIfExpired(key, now);

    auto it = entries.find(key);
    if (it == entries.end()) {
        Entry entry;
        entry.value = "1";
        entry.created_at = now;
        entry.updated_at = now;
        entry.last_accessed_at = now;
        entry.access_count = 1;
        entries[key] = std::move(entry);
        return 1;
    }

    if (!isInteger(it->second.value)) {
        throw std::runtime_error("value is not an integer");
    }

    const int value = stringToInt(it->second.value) + 1;
    it->second.value = std::to_string(value);
    it->second.updated_at = now;
    touchForRead(it->second, now);
    return value;
}

int KVStore::decr(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();
    eraseIfExpired(key, now);

    auto it = entries.find(key);
    if (it == entries.end()) {
        Entry entry;
        entry.value = "-1";
        entry.created_at = now;
        entry.updated_at = now;
        entry.last_accessed_at = now;
        entry.access_count = 1;
        entries[key] = std::move(entry);
        return -1;
    }

    if (!isInteger(it->second.value)) {
        throw std::runtime_error("value is not an integer");
    }

    const int value = stringToInt(it->second.value) - 1;
    it->second.value = std::to_string(value);
    it->second.updated_at = now;
    touchForRead(it->second, now);
    return value;
}

bool KVStore::expire(const std::string& key, int ttl_seconds) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();
    if (eraseIfExpired(key, now)) {
        return false;
    }

    auto it = entries.find(key);
    if (it == entries.end()) {
        return false;
    }

    it->second.expires_at = now + std::chrono::seconds(ttl_seconds);
    it->second.updated_at = now;
    return true;
}

int KVStore::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();
    if (eraseIfExpired(key, now)) {
        return -2;
    }

    const auto it = entries.find(key);
    if (it == entries.end()) {
        return -2;
    }

    const auto ttl_ms = ttlMillis(it->second.expires_at, now);
    if (ttl_ms < 0) {
        return -1;
    }

    return static_cast<int>((ttl_ms + 999) / 1000);
}

std::optional<KeyMetadata> KVStore::metadata(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();
    if (eraseIfExpired(key, now)) {
        return std::nullopt;
    }

    const auto it = entries.find(key);
    if (it == entries.end()) {
        return std::nullopt;
    }

    touchForRead(it->second, now);

    return KeyMetadata{
        key,
        it->second.value.size(),
        toUnixMillis(it->second.created_at),
        toUnixMillis(it->second.updated_at),
        toUnixMillis(it->second.last_accessed_at),
        it->second.access_count,
        ttlMillis(it->second.expires_at, now),
    };
}

std::vector<std::pair<std::string, std::uint64_t>> KVStore::topKeys(size_t limit) {
    std::lock_guard<std::mutex> lock(mtx);
    const auto now = Clock::now();

    for (auto it = entries.begin(); it != entries.end();) {
        if (it->second.expires_at.has_value() && it->second.expires_at.value() <= now) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }

    std::vector<std::pair<std::string, std::uint64_t>> ranked;
    ranked.reserve(entries.size());
    for (const auto& [key, entry] : entries) {
        ranked.emplace_back(key, entry.access_count);
    }

    std::sort(
        ranked.begin(),
        ranked.end(),
        [](const auto& lhs, const auto& rhs) {
            if (lhs.second != rhs.second) {
                return lhs.second > rhs.second;
            }
            return lhs.first < rhs.first;
        }
    );

    if (limit < ranked.size()) {
        ranked.resize(limit);
    }

    return ranked;
}
