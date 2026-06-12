#include "../../inc/core/Connection.hpp"
#include "../../inc/utils/MimeTypes.hpp"
#include "../../inc/utils/Helpers.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>

void Connection::handleDelete(std::string path) {

    if (path.empty()|| _request.getPath().find("..") != std::string::npos)
    {
        buildErrorResponse(400);
        return;
    }
    struct stat st;

    if (stat(path.c_str(), &st) == -1){
        buildErrorResponse(404);
        return;
    }
    if(S_ISDIR(st.st_mode))
    {
        buildErrorResponse(403);
        return;
    }
    if (access(path.c_str(), W_OK) == -1) {
        buildErrorResponse(403);
        return;
    }
    if (unlink(path.c_str()) == 0) {
        _response.setStatusCode(204); 
        _response.setBody(""); 
    } else {
        // OS level failure (e.g., file locked by another process)
        buildErrorResponse(500);
    }
    _header_buffer = _response.getHeadersAsString();
    _is_response_ready = true;
}