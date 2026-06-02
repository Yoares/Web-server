#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/Config.hpp"
#include <string>
#include <map>
#include <vector>
#include <cstring>

class CgiHandler {
public:
    CgiHandler(
        HttpRequest&        request,
        HttpResponse&       response,
        const Location&     location,
        const Server&       server,
        const std::string&  cgi_bin,
        const std::string&  script_path
    );

    void execute();

private:
    HttpRequest&        _request;
    HttpResponse&       _response;
    const Location&     _location;
    const Server&       _server;
    std::string         _cgi_bin;       // e.g. /usr/bin/python3
    std::string         _script_path;   // Physical path to the script file

    std::vector<std::string>    _buildEnv() const;

    char**  _toCharArray(const std::vector<std::string>& vec) const;
    void    _freeCharArray(char** arr, size_t count) const;
    void    _parseCgiOutput(const std::string& raw) const;
    std::string _readAll(int fd) const;
};

#endif