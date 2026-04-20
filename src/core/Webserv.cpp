#include "../../inc/core/Webserv.hpp"

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
	int ready = epoll_wait(epollFd, events.data(), 10, 0);
	if (ready == -1)
		throw std::runtime_error("Fatal Error: epoll_wait failed.");
	else if (ready == 0)
		throw NoEvents();
	else
	{
		events.resize(ready);
		return events;
	}
}
void Webserv::acceptConnections(const std::vector<epoll_event> &events)
{
	for (size_t i = 0; i < events.size(); ++i)
	{
		bool isServerSocket = fdToServers.find(events[i].data.fd) != fdToServers.end();
		if (isServerSocket == false && connections.find(events[i].data.fd) == connections.end())
			connections.insert(std::make_pair(events[i].data.fd, Connection(events[i].data.fd, fdToServers[events[i].data.fd])));
	}
}
void Webserv::handleConnections(const std::vector<epoll_event> &events)
{
	for (size_t i = 0; i < events.size(); ++i)
	{
		if (connections.find(events[i].data.fd) != connections.end())
		{
			// This is a client connection, so we handle it.
		}
	}
}

void Webserv::run()
{
	while (true)
	{
		std::vector<epoll_event> events;
		try
		{
			events = waitforEvents();
		}
		catch (const NoEvents &e)
		{
			continue;
		}
		acceptConnections(events);
		handleConnections(events);
	}
}

Webserv::~Webserv()
{
	for (std::map<int, std::vector<Server> >::iterator it = fdToServers.begin(); it != fdToServers.end(); ++it)
	{
		close(it->first);
	}
	if (epollFd != -1)
		close(epollFd);
}

const char *Webserv::NoEvents::what() const throw()
{
	return "No events to process.";
}