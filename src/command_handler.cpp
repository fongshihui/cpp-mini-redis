#include "../include/command_handler.h"
#include <sstream>

CommandHandler::CommandHandler(KVStore& kv) : store(kv) {}

std::string CommandHandler::handle(const std::string& command) {
    std::stringstream ss(command);

    std::string cmd, key, value;
    ss >> cmd;

    if (cmd == "PING") {
        std::string message;
        std::getline(ss, message);
        if (!message.empty()) {
            message.erase(0, message.find_first_not_of(' '));
            return "PONG " + message;
        }
        return "PONG";
    }

    if (cmd == "SET") {
        ss >> key >> value;
        store.set(key, value);
        return "OK";
    }

    if (cmd == "GET") {
        ss >> key;
        return store.get(key);
    }

    if (cmd == "DEL") {
        ss >> key;
        return store.del(key) ? "1" : "0";
    }

    if (cmd == "EXISTS") {
        ss >> key;
        return store.exists(key) ? "1" : "0";
    }

    if (cmd == "INCR") {
        ss >> key;
        try {
            int result = store.incr(key);
            return std::to_string(result);
        } catch (const std::exception& e) {
            return "ERR value is not an integer or out of range";
        }
    }

    if (cmd == "DECR") {
        ss >> key;
        try {
            int result = store.decr(key);
            return std::to_string(result);
        } catch (const std::exception& e) {
            return "ERR value is not an integer or out of range";
        }
    }

    return "ERR unknown command '" + cmd + "'";
}
