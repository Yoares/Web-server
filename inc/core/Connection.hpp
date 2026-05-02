#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include "../http/HttpRequest.hpp"
#include <sys/socket.h>
#include <exception>
#include <ctime>
#include "../config/Config.hpp"
#include "../http/HttpResponse.hpp"

class Connection {
    private:
        int _client_fd;
        std::vector<Server> _possible_servers;
        const Server* _matched_server; // Starts as NULL
        HttpRequest _request;
		
		time_t _last_activity;
        const Location* matched_location;

        // --- THE HTTP METHOD ROUTING ---
        void handleGet(const Location& loc);
        void handlePost(const Location& loc);
        void handleDelete(const Location& loc);
        std::string resolvePhysicalPath(const std::string& request_uri, const Location& loc);
        void handleDirectory(const std::string& path, const Location& loc);
        void serveFile(const std::string& file_path);
        bool handlePostDir(const std::string& path);

		HttpResponse _response;
        
        std::string _header_buffer;   // Holds the built headers
        size_t _headers_sent;         // Tracks header bytes sent
        size_t _body_sent;            // Tracks body bytes sent
        bool _is_response_ready;

    public:
        Connection(int fd, const std::vector<Server>& servers) 
            : _client_fd(fd), _possible_servers(servers), _matched_server(NULL), 
              _last_activity(time(NULL)), matched_location(NULL), 
              _headers_sent(0), _body_sent(0), _is_response_ready(false) {}
		void handleRequest();

		time_t getLastActivity() const { return _last_activity; }
        void updateActivity() { _last_activity = time(NULL); }

		
		class ConnectionClosed : public std::exception
		{
			public:
				const char* what() const throw();
		};
		bool isResponseReady() const { return _is_response_ready; }
        void sendResponse();

    private:
        const Server* findCorrectServer(const std::string& host);
        // ADDED: Find matching location block
        const Location* findLocation(const Server* server, const std::string& path);
};

#endif