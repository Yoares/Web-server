#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include "ServerBlock.hpp"
#include "LocationBlock.hpp"

class ConfigValidator {
private:
    // Helper functions using POSIX syscalls
    static bool is_directory(const std::string& path);
    static bool is_file_accessible(const std::string& path, int mode);

    static void validate_server(const Server& srv);
    static void validate_location(const Location& loc);

public:
    // The main entry point
    static void validate(const std::vector<Server>& servers);
};

#endif