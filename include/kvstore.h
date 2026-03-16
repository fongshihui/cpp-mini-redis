#pragma once

#include <cctype>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

class KVStore {
private:
    std::unordered_map<std::string, std::string> store;
    std::mutex mtx;

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

public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    int incr(const std::string& key);
    int decr(const std::string& key);
};
