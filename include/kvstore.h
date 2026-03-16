#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

class KVStore {
private:
    std::unordered_map<std::string, std::string> store;
    std::mutex mtx;

    bool isInteger(const std::string& s) {
        if (s.empty()) return false;
        size_t start = 0;
        if (s[0] == '-') start = 1;
        for (size_t i = start; i < s.length(); i++) {
            if (!isdigit(s[i])) return false;
        }
        return true;
    }

    int stringToInt(const std::string& s) {
        try {
            return std::stoi(s);
        } catch (const std::exception&) {
            throw std::runtime_error("value is not an integer");
        }
    }

public:
    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    
    bool exists(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        return store.find(key) != store.end();
    }
    
    int incr(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        if (store.find(key) == store.end()) {
            store[key] = "1";
            return 1;
        }
        
        if (!isInteger(store[key])) {
            throw std::runtime_error("value is not an integer");
        }
        
        int value = stringToInt(store[key]);
        value++;
        store[key] = std::to_string(value);
        return value;
    }
    
    int decr(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        if (store.find(key) == store.end()) {
            store[key] = "-1";
            return -1;
        }
        
        if (!isInteger(store[key])) {
            throw std::runtime_error("value is not an integer");
        }
        
        int value = stringToInt(store[key]);
        value--;
        store[key] = std::to_string(value);
        return value;
    }
};