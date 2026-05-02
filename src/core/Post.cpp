#include "../../inc/core/Connection.hpp"
#include "../../inc/utils/MimeTypes.hpp"
#include "../../inc/utils/Helpers.hpp"
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

static std::string getParentDirectory(
    const std::string& path)
{
    std::string::size_type pos =
        path.find_last_of('/');

    if (pos == std::string::npos)
        return "";

    return path.substr(0, pos);
}

static bool copyFile(int src_fd, int dst_fd)
{
    char buffer[4096];
    ssize_t bytesRead;

    while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t totalWritten = 0;

        while (totalWritten < bytesRead)
        {
            ssize_t written = write(dst_fd, buffer + totalWritten, bytesRead - totalWritten);

            if (written == -1)
                return false;

            totalWritten += written;
        }
    }

    if (bytesRead == -1)
        return false;

    return true;
}

bool Connection::handlePostDir(const std::string& path){
    std::string dir = getParentDirectory(path);


    if (dir.empty())
    {
        _response.buildErrorResponse(400, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return false;
    }

    struct stat st;
    if (stat(dir.c_str(), &st) == -1)
    {
        _response.buildErrorResponse(404, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return false; 
    }
    if (!S_ISDIR(st.st_mode)){
        _response.buildErrorResponse(403, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return false;        
    }
    if (access(dir.c_str(), W_OK) == -1){
        _response.buildErrorResponse(403, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return false;  
    }
    return true;
}

void Connection::handlePost(const Location& loc)
{
    std::string path = resolvePhysicalPath(_request.getPath(), loc);

    if (path.empty())
    {
        _response.buildErrorResponse(400, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    if (!handlePostDir(path))
        return;

    std::string temp_file = _request.getTempFilename();

    struct stat st;
    if (stat(temp_file.c_str(), &st) == -1)
    {
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;       
    }

    if (static_cast<size_t>(st.st_size) > _matched_server->client_max_body_size){

        _response.buildErrorResponse(413, _matched_server->error_pages); // 413 Payload Too Large
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return; 
    }

    int src_fd = open(temp_file.c_str(), O_RDONLY);

    if (src_fd == -1)
    {
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }

    int dst_fd = open(path.c_str(),  O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (dst_fd == -1)
    {
        close(src_fd);
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;       
    }
    if (!copyFile(src_fd, dst_fd))
    {

        close(src_fd);
        close(dst_fd);
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;   
    }
    close(src_fd);
    close(dst_fd);
    unlink(temp_file.c_str());
    _response.setStatusCode(201);
    _response.setBody("Created");

    _header_buffer = _response.getHeadersAsString();
    _is_response_ready = true;
}