#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <map>
#include <sys/epoll.h>
#include "Connection.hpp"
#include <vector>
#include "/home/obendaou/Desktop/Web-server/inc/config/Config.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include "../utils/Helpers.hpp"
#include <exception>

class Webserv
{
    private:
        int epollFd;
        
        std::map <int, std::vector<Server> > fdToServers;

        std::map <int, Connection> connections;

		std::vector<epoll_event> waitforEvents();
		void handleConnections(const std::vector<epoll_event> &events);
		void acceptConnections(const std::vector<epoll_event> &events);

		class NoEvents : public std::exception
		{
			public:
				const char* what() const throw();
		};

    public:
        Webserv(const std::vector<Server> &servers);
        ~Webserv();

        void run(); 
};

#endif