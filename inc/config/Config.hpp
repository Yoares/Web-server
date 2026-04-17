#ifndef CONFIG_HPP
#define CONFIG_HPP
#include <vector>
#include "ServerBlock.hpp"

struct Config
{
    std::vector<Server> servers;
};

#endif