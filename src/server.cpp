#include "../include/server.h"
#include "../include/command_handler.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

bool read_exact(int socket_fd, std::string& out, size_t bytes_to_read) {
    out.clear();
    out.reserve(bytes_to_read);

    while (out.size() < bytes_to_read) {
        char buffer[1024];
        const size_t remaining = bytes_to_read - out.size();
        const size_t chunk_size = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const ssize_t bytes = recv(socket_fd, buffer, chunk_size, 0);
        if (bytes <= 0) {
            return false;
        }
        out.append(buffer, static_cast<size_t>(bytes));
    }

    return true;
}

bool read_line(int socket_fd, std::string& line) {
    line.clear();

    while (true) {
        char ch = '\0';
        const ssize_t bytes = recv(socket_fd, &ch, 1, 0);
        if (bytes <= 0) {
            return false;
        }

        line.push_back(ch);
        const size_t size = line.size();
        if (size >= 2 && line[size - 2] == '\r' && line[size - 1] == '\n') {
            line.resize(size - 2);
            return true;
        }
    }
}

std::vector<std::string> split_inline_command(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }
    return parts;
}

bool read_resp_array(int socket_fd, std::vector<std::string>& args) {
    std::string line;
    if (!read_line(socket_fd, line)) {
        return false;
    }

    if (line.empty()) {
        args.clear();
        return true;
    }

    if (line[0] != '*') {
        args = split_inline_command(line);
        return true;
    }

    int count = 0;
    try {
        count = std::stoi(line.substr(1));
    } catch (const std::exception&) {
        return false;
    }

    if (count < 0) {
        return false;
    }

    args.clear();
    args.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        std::string bulk_length_line;
        if (!read_line(socket_fd, bulk_length_line) || bulk_length_line.empty() ||
            bulk_length_line[0] != '$') {
            return false;
        }

        int bulk_length = 0;
        try {
            bulk_length = std::stoi(bulk_length_line.substr(1));
        } catch (const std::exception&) {
            return false;
        }

        if (bulk_length < 0) {
            return false;
        }

        std::string data;
        if (!read_exact(socket_fd, data, static_cast<size_t>(bulk_length))) {
            return false;
        }

        std::string trailing_crlf;
        if (!read_exact(socket_fd, trailing_crlf, 2) || trailing_crlf != "\r\n") {
            return false;
        }

        args.push_back(std::move(data));
    }

    return true;
}

std::string encode_resp(const RedisReply& reply) {
    switch (reply.type) {
        case RedisReplyType::SimpleString:
            return "+" + reply.value + "\r\n";
        case RedisReplyType::BulkString:
            return "$" + std::to_string(reply.value.size()) + "\r\n" + reply.value + "\r\n";
        case RedisReplyType::Integer:
            return ":" + reply.value + "\r\n";
        case RedisReplyType::Error:
            return "-" + reply.value + "\r\n";
        case RedisReplyType::NullBulkString:
            return "$-1\r\n";
    }

    return "-ERR internal server error\r\n";
}

void handle_client(int client_socket, KVStore& store) {
    CommandHandler handler(store);

    while (true) {
        std::vector<std::string> args;
        if (!read_resp_array(client_socket, args)) {
            break;
        }

        if (args.empty()) {
            const std::string response = "-ERR empty command\r\n";
            send(client_socket, response.c_str(), response.size(), 0);
            continue;
        }

        const std::string response = encode_resp(handler.handle(args));
        send(client_socket, response.c_str(), response.size(), 0);
    }

    close(client_socket);
}

}  // namespace

void Server::start(int port) {
    const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int reuse_addr = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(server_fd, 5) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }

    KVStore store;

    std::cout << "Server started on port " << port << std::endl;

    while (true) {
        const int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            continue;
        }

        std::thread(handle_client, client_socket, std::ref(store)).detach();
    }
}
