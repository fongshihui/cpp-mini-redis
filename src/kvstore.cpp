#include "../include/kvstore.h"

void KVStore::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx);
    store[key] = value;
}

std::string KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);

    if (store.find(key) == store.end())
        return "NULL";

    return store[key];
}

bool KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx);
    return store.erase(key) > 0;
}