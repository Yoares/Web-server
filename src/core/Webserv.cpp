#include "../../inc/core/Webserv.hpp"

extern bool g_server_running;

Webserv::Webserv(const std::vector<Server> &servers) : epollFd(-1)
{
	std::cout << "[INFO] Initializing Webserv..." << std::endl;

	epollFd = epoll_create(10);
	if (epollFd == -1)
	{
		throw std::runtime_error("Fatal Error: epoll_create failed.");
	}

	std::map<std::pair<std::string, int>, int> bound_sockets;

	for (size_t i = 0; i < servers.size(); ++i)
	{

		for (size_t j = 0; j < servers[i].listen_list.size(); ++j)
		{

			int port = servers[i].listen_list[j].port;
			std::string ip = servers[i].listen_list[j].ip;
			std::pair<std::string, int> ip_port_key = std::make_pair(ip, port);

			if (bound_sockets.find(ip_port_key) != bound_sockets.end())
			{
				int existing_fd = bound_sockets[ip_port_key];

				fdToServers[existing_fd].push_back(servers[i]);

				std::cout << "[INFO] Server block mapped to existing FD " << existing_fd
						  << " (" << ip << ":" << port << ")" << std::endl;
				continue;
			}

			std::string port_str = to_string(port);

			struct addrinfo hints, *res;
			std::memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;

			if (getaddrinfo(ip.c_str(), port_str.c_str(), &hints, &res) != 0)
			{
				throw std::runtime_error("Fatal Error: getaddrinfo failed for " + ip + ":" + port_str);
			}

			int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
			if (sockfd == -1)
			{
				freeaddrinfo(res);
				throw std::runtime_error("Fatal Error: socket creation failed.");
			}

			int opt = 1;
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			{
				close(sockfd);
				freeaddrinfo(res);
				throw std::runtime_error("Fatal Error: setsockopt SO_REUSEADDR failed.");
			}

			if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
			{
				close(sockfd);
				freeaddrinfo(res);
				throw std::runtime_error("Fatal Error: bind failed on " + ip + ":" + port_str);
			}

			freeaddrinfo(res);

			if (listen(sockfd, SOMAXCONN) == -1)
			{
				close(sockfd);
				throw std::runtime_error("Fatal Error: listen failed.");
			}

			struct epoll_event ev;
			std::memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLIN;
			ev.data.fd = sockfd;
			if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sockfd, &ev) == -1)
			{
				close(sockfd);
				throw std::runtime_error("Fatal Error: epoll_ctl failed.");
			}

			bound_sockets[ip_port_key] = sockfd;

			fdToServers[sockfd].push_back(servers[i]);

			std::cout << "[SUCCESS] Listening on " << ip << ":" << port
					  << " (FD: " << sockfd << ")" << std::endl;
		}
	}
}
std::vector<epoll_event> Webserv::waitforEvents()
{
	std::vector<epoll_event> events(10);
	int ready = epoll_wait(epollFd, events.data(), 10, 10000);
	if (ready == -1)
	{
		if (errno == EINTR)
			throw NoEvents();
		throw std::runtime_error("Fatal Error: epoll_wait failed.");
	}
	else if (ready == 0)
		throw NoEvents();
	else
	{
		events.resize(ready);
		return events;
	}
}

void Webserv::checkTimeouts()
{
	time_t current_time = time(NULL);
	const int TIMEOUT_LIMIT = 60;        // Idle client timeout
	const int CGI_TIMEOUT_LIMIT = 500;     // Maximum seconds a CGI script is allowed to run

	std::map<int, Connection>::iterator it = connections.begin();
	while (it != connections.end())
	{
		Connection &conn = it->second;

		// ==========================================
		// 1. CGI TIMEOUT CHECK
		// ==========================================
		if (conn._cgi.getState() == CGI_RUNNING)
		{
			// Your existing checkTimeout function kills the process if it exceeds the limit!
			if (conn._cgi.checkTimeout(CGI_TIMEOUT_LIMIT)) 
			{
				std::cout << "[INFO] CGI Timeout triggered for client FD: " << it->first << std::endl;

				// CRITICAL: Remove the dead CGI pipe from epoll so it doesn't infinite loop
				epoll_ctl(epollFd, EPOLL_CTL_DEL, conn._cgi.getStdoutFd(), NULL);

				// Prepare the 504 Gateway Timeout HTML response
				conn.handleCgiTimeout();

				// Modify the client's socket to EPOLLOUT so epoll knows to send the error
				struct epoll_event ev;
				std::memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLOUT;
				ev.data.fd = it->first;
				if (epoll_ctl(epollFd, EPOLL_CTL_MOD, it->first, &ev) == -1) {
					std::cerr << "[ERROR] Failed to modify epoll to EPOLLOUT after CGI timeout" << std::endl;
				}

				it++;
				continue; // Skip the regular timeout check for this loop iteration
			}
		}

		// ==========================================
		// 2. EXISTING CLIENT IDLE TIMEOUT CHECK
		// ==========================================
		if (current_time - conn.getLastActivity() > TIMEOUT_LIMIT)
		{
			std::cout << "[INFO] Connection timed out (FD: " << it->first << "). Closing." << std::endl;

			// Clean up sockets
			epoll_ctl(epollFd, EPOLL_CTL_DEL, it->first, NULL);
			close(it->first);

			std::map<int, Connection>::iterator temp = it;
			it++;
			connections.erase(temp);
		}
		else
		{
			it++;
		}
	}
}

void Webserv::acceptConnections(std::vector<epoll_event> &events)
{
	for (size_t i = 0; i < events.size(); ++i)
	{
		bool isServerSocket = fdToServers.find(events[i].data.fd) != fdToServers.end();
		if (isServerSocket == true)
		{
			int client_fd = accept(events[i].data.fd, NULL, NULL);
			if (client_fd == -1)
			{
				std::cerr << "[ERROR] accept() failed." << std::endl;
				continue;
			}
			struct epoll_event ev;
			std::memset(&ev, 0, sizeof(ev));
			ev.events = EPOLLIN;
			ev.data.fd = client_fd;

			if (epoll_ctl(epollFd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
			{
				close(client_fd);
				continue;
			}
			connections.insert(std::make_pair(client_fd, Connection(client_fd, fdToServers[events[i].data.fd])));
			std::cout << "[INFO] Accepted new connection (FD: " << client_fd << ")" << std::endl;
		}
	}
}

void Webserv::handleConnections(const std::vector<epoll_event> &events)
{
	for (size_t i = 0; i < events.size(); ++i)
	{
		std::map<int, Connection>::iterator it = connections.find(events[i].data.fd);
		if (it != connections.end())
		{
			Connection &conn = it->second;
			try
			{
				if (events[i].events & (EPOLLHUP | EPOLLERR))
				{
					std::cout << "[INFO] Client connection broke unexpectedly (FD: " << events[i].data.fd << ")" << std::endl;
					epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					close(events[i].data.fd);
					connections.erase(it);
					continue;
				}
				if (events[i].events & EPOLLIN)
				{
					if (conn.handleRequest() == 1)
					{
						struct epoll_event ev;
						std::memset(&ev, 0, sizeof(ev));
						ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
						ev.data.fd = conn._cgi.getStdoutFd();
						if (epoll_ctl(epollFd, EPOLL_CTL_ADD, conn._cgi.getStdoutFd(), &ev) == -1)
							throw std::runtime_error("Error modifying epoll for CGI handling");
					}
					if (conn.isResponseReady())
					{
						struct epoll_event ev;
						std::memset(&ev, 0, sizeof(ev));
						ev.events = EPOLLOUT;
						ev.data.fd = events[i].data.fd;
						if (epoll_ctl(epollFd, EPOLL_CTL_MOD, events[i].data.fd, &ev) == -1)
						{
							throw std::runtime_error("Error modifying epoll to EPOLLOUT");
						}
					}
				}
				if (events[i].events & EPOLLOUT)
				{
					conn.sendResponse();
				}
			}
			catch (const Connection::ConnectionClosed &e)
			{
				std::cout << "[INFO] Connection with client closed (FD: " << events[i].data.fd << ")" << std::endl;
				epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				close(events[i].data.fd);
				connections.erase(it);
			}
			catch (const std::exception &e)
			{
				std::cerr << "[ERROR] Error handling FD " << events[i].data.fd << ": " << e.what() << std::endl;
				std::cout << "[INFO] Connection with client closed (FD: " << events[i].data.fd << ")" << std::endl;
				epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				close(events[i].data.fd);
				connections.erase(it);
			}
		}
		else
		{
			std::map<int, Connection>::iterator cgi_it = connections.begin();
			bool found = false;
			while (cgi_it != connections.end())
			{
				if (cgi_it->second._cgi.getStdoutFd() == events[i].data.fd)
				{
					found = true;
					break;
				}
				cgi_it++;
			}
			if (found)
			{
				Connection &conn = cgi_it->second;
				if (events[i].events & EPOLLERR)
				{
					std::cout << "[INFO] CGI process ended unexpectedly for client FD: " << cgi_it->first << std::endl;
					conn._cgi.killProcess();
					epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					conn.buildErrorResponse(500);
					struct epoll_event ev;
					std::memset(&ev, 0, sizeof(ev));
					ev.events = EPOLLOUT;
					ev.data.fd = cgi_it->first;
					if (epoll_ctl(epollFd, EPOLL_CTL_MOD, cgi_it->first, &ev) == -1)
					{
						throw std::runtime_error("Error modifying epoll to EPOLLOUT after CGI completion");
					}
					continue;
				}
				if (events[i].events & (EPOLLIN | EPOLLHUP))
				{
					conn.readCgiOutput();
					if (conn.isResponseReady())
					{
						if (epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
							std::cerr << "[ERROR] Failed to remove CGI FD from epoll." << std::endl;
						}
						struct epoll_event ev;
						std::memset(&ev, 0, sizeof(ev));
						ev.events = EPOLLOUT;
						ev.data.fd = cgi_it->first;
						if (epoll_ctl(epollFd, EPOLL_CTL_MOD, cgi_it->first, &ev) == -1)
						{
							throw std::runtime_error("Error modifying epoll to EPOLLOUT after CGI completion");
						}
					}
				}
			}
		}
	}
}

void Webserv::run()
{
	while (g_server_running)
	{
		std::vector<epoll_event> events;
		try
		{
			events = waitforEvents();
		}
		catch (const NoEvents &e)
		{
			checkTimeouts();
			continue;
		}
		acceptConnections(events);
		handleConnections(events);
		checkTimeouts();
	}
}

Webserv::~Webserv()
{
	std::cout << "[INFO] Cleaning up resources..." << std::endl;

	for (std::map<int, Connection>::iterator it = connections.begin(); it != connections.end(); ++it)
	{
		close(it->first);
	}
	connections.clear();

	for (std::map<int, std::vector<Server> >::iterator it = fdToServers.begin(); it != fdToServers.end(); ++it)
	{
		close(it->first);
	}
	fdToServers.clear();

	if (epollFd != -1)
	{
		close(epollFd);
	}

	std::cout << "[INFO] Server shutdown complete." << std::endl;
}

const char *Webserv::NoEvents::what() const throw()
{
	return "No events to process.";
}