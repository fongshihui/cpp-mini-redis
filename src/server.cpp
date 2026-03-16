#include "../include/server.h"
#include "../include/command_handler.h"

#include <iostream>
#include <thread>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void handle_client(int client_socket, KVStore& store) {

    CommandHandler handler(store);

    char buffer[1024];

    while (true) {
        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes <= 0)
            break;

        std::string command(buffer, bytes);

        std::string response = handler.handle(command);

        send(client_socket, response.c_str(), response.size(), 0);
    }

    close(client_socket);
}

void Server::start(int port) {

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    KVStore store;

    std::cout << "Server started on port " << port << std::endl;

    while (true) {

        int client_socket = accept(server_fd, nullptr, nullptr);

        std::thread(handle_client, client_socket, std::ref(store)).detach();
    }
}