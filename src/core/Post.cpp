#include "../../inc/core/Post.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>

std::string PostHandler::resolvePhysicalPath(const std::string& request_uri, const Location& loc) {
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

static std::string getParentDirectory(
    const std::string& path)
{
    std::string::size_type pos =
        path.find_last_of('/');

    if (pos == std::string::npos)
        return "";

    return path.substr(0, pos);
}

PostHandler::PostHandler(HttpRequest& request, HttpResponse& response, const Server& server, const Location& location)
    : _request(request),
      _response(response),
      _server(server),
      _location(location)
{

}

bool PostHandler::validateBodySize(const std::string& temp_file) {
    struct stat st;
    if (stat(temp_file.c_str(), &st) == -1) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;       
    }

    if (static_cast<size_t>(st.st_size) > _server.client_max_body_size) {
        _response.buildErrorResponse(413, _server.error_pages); // 413 Payload Too Large
        return false; 
    }
    return true;
}

bool PostHandler::validateUploadDirectory(const std::string& path){
    std::string dir = getParentDirectory(path);


if (dir.empty()) {
        _response.buildErrorResponse(400, _server.error_pages);
        return false;
    }

    struct stat st;
    if (stat(dir.c_str(), &st) == -1) {
        _response.buildErrorResponse(404, _server.error_pages);
        return false; 
    }
    if (!S_ISDIR(st.st_mode)) {
        _response.buildErrorResponse(403, _server.error_pages);
        return false;        
    }
    if (access(dir.c_str(), W_OK) == -1) {
        _response.buildErrorResponse(403, _server.error_pages);
        return false;  
    }
    return true;
}

bool PostHandler::copyToDestination(const std::string& temp_file, const std::string& path) {
    int src_fd = open(temp_file.c_str(), O_RDONLY);
    if (src_fd == -1) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }

    int dst_fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dst_fd == -1) {
        close(src_fd);
        _response.buildErrorResponse(500, _server.error_pages);
        return false;       
    }

    char buffer[4096];
    ssize_t bytesRead;
    bool success = true;

    while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t totalWritten = 0;
        while (totalWritten < bytesRead) {
            ssize_t written = write(dst_fd, buffer + totalWritten, bytesRead - totalWritten);
            if (written == -1) {
                success = false;
                break;
            }
            totalWritten += written;
        }
        if (!success) break;
    }

    if (bytesRead == -1) {
        success = false;
    }

    close(src_fd);
    close(dst_fd);

    if (!success) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;   
    }

    return true;
}

bool PostHandler::isMultipart() const{
    std::map<std::string, std::string> headers = _request.getHeaders();

    std::map<std::string, std::string>::iterator it = headers.find("Content-Type");

    if (it == headers.end())
        return false;
    
    return (it->second.find("multipart/form-data")
        != std::string::npos);
}

std::string PostHandler::extractBoundary(const std::string& contentType) const{
    std::string key = "boundary=";

    std::string::size_type pos = contentType.find(key);

    if (pos == std::string::npos)
        return "";

    return contentType.substr(pos + key.length());
}

void PostHandler::execute() {
    std::string path = resolvePhysicalPath(_request.getPath(), _location);

    if (path.empty()) {
        _response.buildErrorResponse(400, _server.error_pages);
        return;
    }

    // 1. Check directory permissions
    if (!validateUploadDirectory(path)) {
        return;
    }

    std::string temp_file = _request.getTempFilename();

    // 2. Validate max body size
    if (!validateBodySize(temp_file)) {
        return;
    }

    // NOTE: If handling multipart forms in the future, inject parsing logic here
    if (isMultipart())
    {
        // multipart flow
    }
    // 3. Move the physical file
    else {

        if (!copyToDestination(temp_file, path)) {
            return;
        }
    }

    // Optional: unlink(temp_file.c_str());

    // 4. Success Response
    _response.setStatusCode(201);
    _response.setBody("Created");
}
