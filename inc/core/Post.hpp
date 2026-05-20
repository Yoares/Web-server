#ifndef POSTHANDLER_HPP
#define POSTHANDLER_HPP

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../config/Config.hpp"
#include <vector>
#include <fstream>
#include <algorithm>

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
    
    // UPDATE THIS SIGNATURE (Add out_filename):
    bool processMultipart(const std::string& temp_file, const std::string& boundary, const std::string& upload_dir, std::string& out_filename);
    
    // ADD THIS NEW METHOD:
    void buildSuccessResponse(const std::vector<std::string>& finalNames, bool isRaw);

};

#endif