#include "../../inc/core/Post.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>

// handlePost()
//     |
//     |-- resolve upload path
//     |-- open file
//     |-- write body
//     |-- send success response
//handle multipart
//temp file
//    ↓
// parse multipart
//    ↓
// extract real file content
//    ↓
// save file

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

    // 3. Move the physical file
    if (!copyToDestination(temp_file, path)) {
        return;
    }

    // Optional: unlink(temp_file.c_str());

    // 4. Success Response
    _response.setStatusCode(201);
    _response.setBody("Created");
}


// static bool copyFile(int src_fd, int dst_fd)
// {
//     char buffer[4096];
//     ssize_t bytesRead;

//     while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0)
//     {
//         ssize_t totalWritten = 0;

//         while (totalWritten < bytesRead)
//         {
//             ssize_t written = write(dst_fd, buffer + totalWritten, bytesRead - totalWritten);

//             if (written == -1)
//                 return false;

//             totalWritten += written;
//         }
//     }

//     if (bytesRead == -1)
//         return false;

//     return true;
// }

// bool Connection::handlePostDir(const std::string& path){
//     std::string dir = getParentDirectory(path);


//     if (dir.empty())
//     {
//         _response.buildErrorResponse(400, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return false;
//     }

//     struct stat st;
//     if (stat(dir.c_str(), &st) == -1)
//     {
//         _response.buildErrorResponse(404, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return false; 
//     }
//     if (!S_ISDIR(st.st_mode)){
//         _response.buildErrorResponse(403, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return false;        
//     }
//     if (access(dir.c_str(), W_OK) == -1){
//         _response.buildErrorResponse(403, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return false;  
//     }
//     return true;
// }

// static std::string extractBoundary(const std::string& content_type){
//     std::string key = "boundry";

//     size_t pos = content_type.find(key);

//     if (pos == std::string::npos)
//         return "";
//     return (content_type.substr(pos + key.length()));
// }

// void Connection::handlePost(const Location& loc)
// {
//     std::string path = resolvePhysicalPath(_request.getPath(), loc);

//     if (path.empty())
//     {
//         _response.buildErrorResponse(400, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return;
//     }
//     if (!handlePostDir(path))
//         return;

//     std::string temp_file = _request.getTempFilename();

//     struct stat st;
//     if (stat(temp_file.c_str(), &st) == -1)
//     {
//         _response.buildErrorResponse(500, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return;       
//     }

//     if (static_cast<size_t>(st.st_size) > _matched_server->client_max_body_size){

//         _response.buildErrorResponse(413, _matched_server->error_pages); // 413 Payload Too Large
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return; 
//     }
//     // start pasing the multipart
//     std::map<std::string, std::string> headrs = _request.getHeaders();

//     std::string content_type = headrs["Content-Type"];

//     if (content_type.find("multipart/form-data") != std::string::npos){

//     }
    
//     int src_fd = open(temp_file.c_str(), O_RDONLY);

//     if (src_fd == -1)
//     {
//         _response.buildErrorResponse(500, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return;
//     }

//     int dst_fd = open(path.c_str(),  O_CREAT | O_WRONLY | O_TRUNC, 0644);

//     if (dst_fd == -1)
//     {
//         close(src_fd);
//         _response.buildErrorResponse(500, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return;       
//     }
//     if (!copyFile(src_fd, dst_fd))
//     {

//         close(src_fd);
//         close(dst_fd);
//         _response.buildErrorResponse(500, _matched_server->error_pages);
//         _header_buffer = _response.getHeadersAsString();
//         _is_response_ready = true;
//         return;   
//     }
//     close(src_fd);
//     close(dst_fd);
//     // unlink(temp_file.c_str());
//     _response.setStatusCode(201);
//     _response.setBody("Created");

//     _header_buffer = _response.getHeadersAsString();
//     _is_response_ready = true;
// }