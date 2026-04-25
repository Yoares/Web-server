#include "../../inc/utils/MimeTypes.hpp"

std::string MimeTypes::getMimeType(const std::string& path)
{
    std::string::size_type dot = path.find_last_of('.');

    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);

    if (ext == ".html" || ext == ".htm")
        return "text/html";

    if (ext == ".css")
        return "text/css";

    if (ext == ".js")
        return "application/javascript";

    if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";

    if (ext == ".png")
        return "image/png";

    if (ext == ".gif")
        return "image/gif";

    if (ext == ".txt")
        return "text/plain";

    return "application/octet-stream";
}