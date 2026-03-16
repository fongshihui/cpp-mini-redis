#include "../include/kvstore.h"

void KVStore::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx);
    store[key] = value;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = store.find(key);
    if (it == store.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    return store.erase(key) > 0;
}

bool KVStore::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    return store.find(key) != store.end();
}

int KVStore::incr(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = store.find(key);
    if (it == store.end()) {
        store[key] = "1";
        return 1;
    }

    if (!isInteger(it->second)) {
        throw std::runtime_error("value is not an integer");
    }

    int value = stringToInt(it->second) + 1;
    it->second = std::to_string(value);
    return value;
}

int KVStore::decr(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = store.find(key);
    if (it == store.end()) {
        store[key] = "-1";
        return -1;
    }

    if (!isInteger(it->second)) {
        throw std::runtime_error("value is not an integer");
    }

    int value = stringToInt(it->second) - 1;
    it->second = std::to_string(value);
    return value;
}
