#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include "../config/Config.hpp"
#include "../utils/Helpers.hpp"

class HttpResponse {
private:
    int _status_code;
    std::string _status_message;
    std::map<std::string, std::string> _headers;
    
    // For small text responses (like error pages)
    std::string _body;
    
    // For large file responses
    bool _is_file;
    std::string _file_path;
    size_t _file_size;

    std::string getReasonPhrase(int code) const;

public:
    HttpResponse();
    ~HttpResponse();

    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    
    // ADDED: Setup for serving a large file
    void setFile(const std::string& path, size_t size);

    void buildErrorResponse(int code, const std::map<int, std::string>& error_pages);
    std::string getHeadersAsString() const;

    // Getters for the Connection class to use
    bool isFile() const { return _is_file; }
    std::string getFilePath() const { return _file_path; }
    size_t getFileSize() const { return _file_size; }
    std::string getBody() const { return _body; }
};

#endif