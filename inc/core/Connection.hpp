#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include "../http/HttpRequest.hpp"

class Connection {
    private:
        int _client_fd;
        std::vector<Server> _possible_servers;
        const Server* _matched_server; // Starts as NULL
        HttpRequest _request;

    public:
        Connection(int fd, const std::vector<Server>& servers) 
            : _client_fd(fd), _possible_servers(servers), _matched_server(NULL) {}

    private:
        const Server* findCorrectServer(const std::string& host);
};

#endif