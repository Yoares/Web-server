#include "../../inc/core/Connection.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

std::string Connection::resolvePhysicalPath(const std::string& request_uri, const Location& loc) {
    std::string physical_path;

    // NGINX 'root' behavior: Append the entire URI to the root path.
    // E.g., root is "/var/www", URI is "/images/cat.jpg" -> "/var/www/images/cat.jpg"
    
    // 1. Get the root from the location (fallback to a default if empty)
    std::string root = loc.root.empty() ? "/var/www/html" : loc.root;

    // 2. Prevent double slashes when joining
    if (root[root.length() - 1] == '/' && request_uri[0] == '/') {
        physical_path = root + request_uri.substr(1);
    } else if (root[root.length() - 1] != '/' && request_uri[0] != '/') {
        physical_path = root + "/" + request_uri;
    } else {
        physical_path = root + request_uri;
    }

    return physical_path;
}

void Connection::handleGet(const Location& loc) {
    std::string physical_path = resolvePhysicalPath(_request.getPath(), loc);

    if (physical_path.empty())
        // throw HttpException(400);
    
    struct stat st;
    if (lstat(physical_path.c_str(), &st) == -1){
        // handleStatError(errno);
    }
    if (S_ISLNK(st.st_mode))
        // throw HttpException(403);

    if (S_ISDIR(st.st_mode)){
        std::string req_path = _request.getPath();
        
    }
}