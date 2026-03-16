#include "../include/command_handler.h"
#include <sstream>

CommandHandler::CommandHandler(KVStore& kv) : store(kv) {}

std::string CommandHandler::handle(const std::string& command) {
    std::stringstream ss(command);

    std::string cmd, key, value;
    ss >> cmd;

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
        return store.del(key) ? "DELETED" : "NOT_FOUND";
    }

    return "UNKNOWN_COMMAND";
}