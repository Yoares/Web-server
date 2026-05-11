#include "../../inc/core/Connection.hpp"
#include "../../inc/utils/MimeTypes.hpp"
#include "../../inc/utils/Helpers.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>

void Connection::handleDelete(const Location& loc) {
    std::string path = resolvePhysicalPath(_request.getPath(), loc);

    if (path.empty()|| _request.getPath().find("..") != std::string::npos)
    {
        _response.buildErrorResponse(400, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    struct stat st;

    if (stat(path.c_str(), &st) == -1){
        _response.buildErrorResponse(404, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    if(S_ISDIR(st.st_mode))
    {
        _response.buildErrorResponse(403, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    if (access(path.c_str(), W_OK) == -1) {
        _response.buildErrorResponse(403, _matched_server->error_pages); // Forbidden
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    if (unlink(path.c_str()) == 0) {
        _response.setStatusCode(204); 
        _response.setBody(""); 
    } else {
        // OS level failure (e.g., file locked by another process)
        _response.buildErrorResponse(500, _matched_server->error_pages);
    }
    _header_buffer = _response.getHeadersAsString();
    _is_response_ready = true;
}