#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include "../http/HttpRequest.hpp"
#include <sys/socket.h>
#include <exception>
#include <ctime>
#include "../config/Config.hpp"
#include "../http/HttpResponse.hpp"
#include "cgi/CgiHandler.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../utils/Logger.hpp"
#include <unistd.h>

class Logger_manager;

class Connection
{
	private:
		int _client_fd;
		std::vector<Server> _possible_servers;
		Logger_manager *_session_manager;
		const Server *_matched_server;
		HttpRequest _request;
		Location _fall_back_location;

		time_t _last_activity;
		const Location *matched_location;
		int listen_port;

		HttpResponse _response;

		std::string _header_buffer;
		size_t _headers_sent;
		size_t _body_sent;
		bool _is_response_ready;

		void handleGet(const Location &loc, std::string _path);
		void handlePost(const Location &loc, std::string _path);
		void handleDelete(std::string _path);
		std::string resolvePhysicalPath(const std::string &request_uri, const Location &loc);
		void handleDirectory(const std::string &path, const Location &loc);
		void serveFile(const std::string &file_path);
		int checkCGI(const std::string &path);
		std::vector<std::string> buildCgiEnv(const std::string &physical_path);
		void parseCgiHeaders(const std::string &headers_str);
		const Server *findCorrectServer(const std::string &host);

		const Location *findLocation(const Server *server, const std::string &path);

	public:
		class ConnectionClosed : public std::exception
		{
		public:
			const char *what() const throw();
		};
		
		Connection(int fd, const std::vector<Server> &servers, Logger_manager *_session_manager);
		int handleRequest();
		void updateActivity();
		void readCgiOutput();
		time_t getLastActivity();
		void handleCgiTimeout();
		void buildErrorResponse(int code);
		bool isResponseReady();
		void sendResponse();

		CgiHandler _cgi;
};

#endif