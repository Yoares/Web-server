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
    std::string root ;

    if (loc.root.empty())
        root = "/var/www/html";
    else
        root = loc.root;

    if (request_uri.find("..") != std::string::npos)
        return "";

    // Prevent double slashes when joining
    if (root[root.length() - 1] == '/' && request_uri[0] == '/') {
        _path = root + request_uri.substr(1);
    } else if (root[root.length() - 1] != '/' && request_uri[0] != '/') {
        _path = root + "/" + request_uri;
    } else {
        _path = root + request_uri;
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
            _response.buildErrorResponse(500, _matched_server->error_pages);
            _header_buffer = _response.getHeadersAsString();
            _is_response_ready = true;
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
        _response.buildErrorResponse(403, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    
}

void Connection::serveFile(const std::string& _path) {

    if (access(_path.c_str(), R_OK) == -1)
    {
        _response.buildErrorResponse(403, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }

    int fd = open(_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }

    std::string content;
    if (!readFile(fd, content))
    {
        close(fd);
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
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

void Connection::handleGet(const Location& loc) {

    std::string _path = resolvePhysicalPath(_request.getPath(), loc);

    if (_path.empty() || _request.getPath().find("..") != std::string::npos)
    {
        _response.buildErrorResponse(400, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    struct stat lst;
    if (lstat(_path.c_str(), &lst) == -1) // symlinks protection
    {   
        if(errno == ENOENT)
            _response.buildErrorResponse(404, _matched_server->error_pages);
        else if (errno == EACCES)
            _response.buildErrorResponse(403, _matched_server->error_pages);
        else
            _response.buildErrorResponse(500, _matched_server->error_pages);
        
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return ;
    }

    if (S_ISLNK(lst.st_mode)){
         _response.buildErrorResponse(403, _matched_server->error_pages);
         _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return ;
    }
    struct stat st;
    if (stat(_path.c_str(), &st) == -1){
        if(errno == ENOENT)
            _response.buildErrorResponse(404, _matched_server->error_pages);
        else if (errno == EACCES)
            _response.buildErrorResponse(403, _matched_server->error_pages);
        else
            _response.buildErrorResponse(500, _matched_server->error_pages);

        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return ;
    }

    // Directory handling
    if (S_ISDIR(st.st_mode))
    {
        handleDirectory(_path, loc);
        return;
    }
    
    // Only regular files allowed
    if (!S_ISREG(st.st_mode))
    {
        _response.buildErrorResponse(403, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }

    serveFile(_path);
}