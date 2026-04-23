#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include "../http/HttpRequest.hpp"
#include <sys/socket.h>
#include <exception>
#include <ctime>
#include "../config/Config.hpp"

class Connection {
    private:
        int _client_fd;
        std::vector<Server> _possible_servers;
        const Server* _matched_server; // Starts as NULL
        HttpRequest _request;
		time_t _last_activity;

    public:
        Connection(int fd, const std::vector<Server>& servers) 
            : _client_fd(fd), _possible_servers(servers), _matched_server(NULL) {}
		void handleRequest();

		time_t getLastActivity() const { return _last_activity; }
        void updateActivity() { _last_activity = time(NULL); }

		
		class ConnectionClosed : public std::exception
		{
			public:
				const char* what() const throw();
		};

    private:
        const Server* findCorrectServer(const std::string& host);
        // ADDED: Find matching location block
        const Location* findLocation(const Server* server, const std::string& path);
};

#endif