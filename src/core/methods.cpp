#include "../../inc/core/Connection.hpp"
#include "../../inc/utils/MimeTypes.hpp"
#include "../../inc/utils/Helpers.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>

std::string Connection::resolvePhysicalPath(const std::string& request_uri, const Location& loc) {
    std::string physical_path;

    std::string root = loc.root.empty() ? "/var/www/html" : loc.root;

    // Prevent double slashes when joining
    if (root[root.length() - 1] == '/' && request_uri[0] == '/') {
        physical_path = root + request_uri.substr(1);
    } else if (root[root.length() - 1] != '/' && request_uri[0] != '/') {
        physical_path = root + "/" + request_uri;
    } else {
        physical_path = root + request_uri;
    }

    return physical_path;
}

static std::string readFile(int fd){
    std::string content;
    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
        content.append(buffer, bytesRead);
    }

    return content;
}

void Connection::handleGet(const Location& loc) {
    std::string _path = resolvePhysicalPath(_request.getPath(), loc);

    struct stat st;
    if (stat(_path.c_str(), &st) == -1)
        throw 404;
    //is a regular file
    if (S_ISDIR(st.st_mode)){
        int fd = open(_path.c_str(), O_RDONLY);
        if (fd == -1)
        {
            _response.buildErrorResponse(403, _matched_server->error_pages);
            _header_buffer = _response.getHeadersAsString();
            _is_response_ready = true;
            return ;
        }
        std::string content = readFile(fd);

        _response.setStatusCode(200);
        _response.setHeader("Content-Length", to_string(content.size()));
        _response.setHeader("Content-Type", MimeTypes::getMimeType(_path));
        _response.setBody(content);
    }
    else
    {
        _response.buildErrorResponse(403, _matched_server->error_pages);
            _header_buffer = _response.getHeadersAsString();
            _is_response_ready = true;
            return ;
    }
}