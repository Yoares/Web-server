#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationBlock.hpp"

// Assuming 'struct Location' is defined above this
// 1. Create a tiny struct to hold the pair
struct ListenParams {
    std::string ip;
    int port;
    // int socket_fd; // Moving the socket_fd here because every port gets its own socket!

    // Constructor
    ListenParams(std::string i, int p) : ip(i), port(p) {}
};

struct Server {
    // Remove the old single ip, port, and socket_fd:
    // std::string ip;
    // int port;
    // int socket_fd;

    // 2. Replace them with a vector of your new struct:
    std::vector<ListenParams> listen_list;

    std::vector<std::string> server_names;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<Location> locations;

    Server() : client_max_body_size(1048576) {}
};

#endif