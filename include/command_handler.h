#pragma once
#include <string>
#include "kvstore.h"

class CommandHandler {
private:
    KVStore& store;

public:
    CommandHandler(KVStore& kv);
    std::string handle(const std::string& command);
};