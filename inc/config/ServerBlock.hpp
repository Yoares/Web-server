#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationBlock.hpp"

// Assuming 'struct Location' is defined above this

struct Server
{
    std::string ip;
    int port;
    std::vector<std::string> server_names;
    
    size_t client_max_body_size;
    
    std::map<int, std::string> error_pages;
    
    std::vector<Location> locations;
    int socket_fd;

};

#endif