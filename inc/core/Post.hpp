#ifndef POSTHANDLER_HPP
#define POSTHANDLER_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/Config.hpp"

class PostHandler
{
private:
    HttpRequest& _request;
    HttpResponse& _response;
    const Server& _server;
    const Location& _location;

public:
    PostHandler(
        HttpRequest& request,
        HttpResponse& response,
        const Server& server,
        const Location& location);

    void execute();

private:
    std::string resolvePhysicalPath(const std::string& request_uri, const Location& loc);
    bool validateUploadDirectory(const std::string& path);
    bool validateBodySize(const std::string& temp_file);
    bool copyToDestination(const std::string& temp_file,const std::string& path);
    // multipart
    bool isMultipart() const;
    std::string extractBoundary(const std::string& contentType) const;
    bool readTempFile(const std::string& temp_file, std::string& body);
    bool extractMultipartContent(const std::string& body, const std::string& boundary, std::string& file_content);
};

#endif