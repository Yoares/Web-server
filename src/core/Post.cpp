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
    int fd = open(path.c_str(),  O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fd == -1)
    {
        _response.buildErrorResponse(500, _matched_server->error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;       
    }
    // std::string body = _request.get 
}