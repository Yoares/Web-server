#include "../../inc/core/Connection.hpp"
#include "../../inc/utils/MimeTypes.hpp"
#include "../../inc/utils/Helpers.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>

std::string Connection::resolvePhysicalPath(const std::string& request_uri, const Location& loc) {
    std::string _path;
    std::string root;

    if (loc.root.empty())
        root = "/var/www/html";
    else
        root = loc.root;

    if (request_uri.find("..") != std::string::npos)
        return "";

    // --- ALIAS BEHAVIOR: Strip the location path from the URI ---
    std::string clean_uri = request_uri;
    if (!loc.path.empty() && request_uri.find(loc.path) == 0) {
        clean_uri = request_uri.substr(loc.path.length());
        if (clean_uri.empty() || clean_uri[0] != '/') {
            clean_uri = "/" + clean_uri;
        }
    }

    // Prevent double slashes when joining
    if (root[root.length() - 1] == '/' && clean_uri[0] == '/') {
        _path = root + clean_uri.substr(1);
    } else if (root[root.length() - 1] != '/' && clean_uri[0] != '/') {
        _path = root + "/" + clean_uri;
    } else {
        _path = root + clean_uri;
    }

    return _path;
}

static bool readFile(int fd, std::string &content){

    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
        content.append(buffer, bytesRead);
    }
    if (bytesRead == -1)
        return false;
    return true;
}

void Connection::handleDirectory(const std::string& path, const Location& loc){

    if (!loc.index.empty()){
        std::string index_path = path;
        if (index_path[index_path.length() - 1] != '/'){
            index_path += "/";
        }
        index_path += loc.index;
        struct  stat st;
        if (stat(index_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)){
            serveFile(index_path);
            return;
        }
    }
    if (loc.autoindex){
        DIR *dir = opendir(path.c_str());
        if (dir == NULL){
            buildErrorResponse(500);
            return;
        }
        std::string body;
        body += "<html><body><ul>";

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL)
        {
            body += "<li>";
            body += entry->d_name;
            body += "</li>";
        }

        body += "</ul></body></html>";

        closedir(dir);
        _response.setStatusCode(200);
        _response.setHeader("Content-type", "text/html");
        _response.setHeader("Content-Length", to_string(body.size()));
        _response.setBody(body);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    else { //this if the autoindex is off
        buildErrorResponse(404);
        return;
    }
    
}

void Connection::serveFile(const std::string& _path) {

    if (access(_path.c_str(), R_OK) == -1)
    {
        buildErrorResponse(403);
        return;
    }

    int fd = open(_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        buildErrorResponse(500);
        return;
    }

    std::string content;
    if (!readFile(fd, content))
    {
        close(fd);
        buildErrorResponse(500);
        return;
    }

    close(fd);
    _response.setStatusCode(200);
    _response.setHeader("Content-Length", to_string(content.size()));
    _response.setHeader("Content-Type", MimeTypes::getMimeType(_path));
    _response.setBody(content);
    _header_buffer = _response.getHeadersAsString();
    _is_response_ready = true;
    return;
}

void Connection::handleGet(const Location& loc, std::string _path) {

    if (_path.empty() || _request.getPath().find("..") != std::string::npos)
    {
        buildErrorResponse(400);
        return;
    }
    struct stat lst;
    if (lstat(_path.c_str(), &lst) == -1) // symlinks protection
    {   
        if(errno == ENOENT)
            buildErrorResponse(404);
        else if (errno == EACCES)
            buildErrorResponse(403);
        else
            buildErrorResponse(500);
        return ;
    }

    if (S_ISLNK(lst.st_mode)){
         buildErrorResponse(403);
        return ;
    }
    struct stat st;
    if (stat(_path.c_str(), &st) == -1){
        if(errno == ENOENT)
            buildErrorResponse(404);
        else if (errno == EACCES)
            buildErrorResponse(403);
        else
            buildErrorResponse(500);
        return ;
    }

    // Directory handling
    if (S_ISDIR(st.st_mode))
    {
		std::string req_path = _request.getPath();
        if (!req_path.empty() && req_path[req_path.length() - 1] != '/') {
            _response.setStatusCode(301);
            _response.setHeader("Location", req_path + "/");
            _response.setHeader("Content-Type", "text/html");
			_response.setHeader("Connection", "close");
            _response.setBody("<html><body><h1>301 Moved Permanently</h1></body></html>");
            
            _header_buffer = _response.getHeadersAsString();
            _is_response_ready = true;
            return;
        }
        handleDirectory(_path, loc);
        return;
    }
    
    // Only regular files allowed
    if (!S_ISREG(st.st_mode))
    {
        buildErrorResponse(403);
        return;
    }

    serveFile(_path);
}