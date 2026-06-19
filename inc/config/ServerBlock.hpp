#ifndef SERVERBLOCK_HPP
#define SERVERBLOCK_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationBlock.hpp"

struct ListenParams
{
	std::string ip;
	int port;

	ListenParams(std::string i, int p) : ip(i), port(p) {}
};

struct Server
{

	std::vector<ListenParams> listen_list;

	std::vector<std::string> server_names;
	size_t client_max_body_size;
	std::string root;
	std::string index;
	std::map<int, std::string> error_pages;
	std::vector<Location> locations;

	Server() : client_max_body_size(1048576) {}
};

#endif