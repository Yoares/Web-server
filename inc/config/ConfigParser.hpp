#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include "ServerBlock.hpp"
#include "LocationBlock.hpp"

class ConfigParser
{
	private:
		std::vector<std::string> _tokens;
		size_t _pos;
		std::string consume();
		void expect(const std::string &expected);

		Server parse_server();
		Location parse_location(size_t inherited_max_body_size, std::string &inher_root, std::string &inher_index);

	public:
		ConfigParser(const std::vector<std::string> &tokens);
		std::vector<Server> parse();
};

#endif