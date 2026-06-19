#ifndef LOCATIONBLOCK_HPP
#define LOCATIONBLOCK_HPP
#include <string>
#include <vector>
#include <map>
#include <iostream>

struct Location
{
	Location();
	std::string path;
	std::string root;
	std::string index;
	size_t client_max_body_size;

	std::vector<std::string> allowed_methods;

	bool autoindex;
	std::string upload_dir;

	int redirect_code;
	std::string redirect_url;

	std::map<std::string, std::string> cgi_pass;
};

#endif