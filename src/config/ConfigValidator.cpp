#include "../../inc/config/ConfigValidator.hpp"
#include <sys/stat.h>
#include <unistd.h>

void ConfigValidator::validate(const std::vector<Server> &servers)
{

    if (servers.empty())
    {
        throw std::runtime_error("Validation Error: No servers configured.");
    }

    for (size_t i = 0; i < servers.size(); ++i)
    {
        validate_server(servers[i]);
    }
}

void ConfigValidator::validate_server(const Server &srv)
{

    if (srv.listen_list.empty())
    {
        throw std::runtime_error("Validation Error: Server block is missing a 'listen' directive.");
    }

    for (size_t i = 0; i < srv.listen_list.size(); ++i)
    {
        if (srv.listen_list[i].port <= 0 || srv.listen_list[i].port > 65535)
        {

            throw std::runtime_error("Validation Error: Invalid port number detected in a listen directive.");
        }
    }

    for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin();
         it != srv.error_pages.end(); ++it)
    {
        if (!is_file_accessible(it->second, R_OK))
        {
            throw std::runtime_error("Validation Error: Error page not found or unreadable: " + it->second);
        }
    }

    for (size_t i = 0; i < srv.locations.size(); ++i)
    {
        validate_location(srv.locations[i]);
    }
}

void ConfigValidator::validate_location(const Location &loc)
{
    if (!loc.root.empty() && !is_directory(loc.root))
    {
        throw std::runtime_error("Validation Error: Root directory does not exist or is not a directory: " + loc.root);
    }

    for (size_t i = 0; i < loc.allowed_methods.size(); ++i)
    {
        const std::string &method = loc.allowed_methods[i];
        if (method != "GET" && method != "POST" && method != "DELETE")
        {
            throw std::runtime_error("Validation Error: Unsupported HTTP method: " + method);
        }
    }

    for (std::map<std::string, std::string>::const_iterator it = loc.cgi_pass.begin();
         it != loc.cgi_pass.end(); ++it)
    {
        if (!is_file_accessible(it->second, X_OK))
        {
            throw std::runtime_error("Validation Error: CGI binary not found or not executable: " + it->second);
        }
    }

    if (!loc.upload_dir.empty() && !is_directory(loc.upload_dir))
    {

        if (mkdir(loc.upload_dir.c_str(), 0777) == -1)
        {
            throw std::runtime_error("Validation Error: Upload directory does not exist and could not be created (Check permissions): " + loc.upload_dir);
        }
        else
        {
            std::cout << "[INFO] Created missing upload directory at boot: " << loc.upload_dir << std::endl;
        }
    }
}

bool ConfigValidator::is_directory(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return S_ISDIR(info.st_mode);
}

bool ConfigValidator::is_file_accessible(const std::string &path, int mode)
{
    return (access(path.c_str(), mode) == 0);
}