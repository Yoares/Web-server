#include "../../inc/http/HttpResponse.hpp"
#include <iostream>

HttpResponse::HttpResponse() : _status_code(200), _status_message("OK"), _body(""), _is_file(false), _file_size(0) {}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatusCode(int code) {
    _status_code = code;
    _status_message = getReasonPhrase(code);
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body;
    _is_file = false;
    setHeader("Content-Length", to_string(_body.length()));
}

void HttpResponse::setFile(const std::string& path, size_t size) {
    _file_path = path;
    _file_size = size;
    _is_file = true;
    setHeader("Content-Length", to_string(size));
}

std::string HttpResponse::getReasonPhrase(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 404: return "Not Found";
		case 413: return "Payload Too Large";
		case 400: return "Bad Request";
		case 500: return "Internal Server Error";
		case 505: return "HTTP Version Not Supported";
		case 501: return "Not Implemented";
		case 411: return "Length Required";
		case 431: return "Request Header Fields Too Large";
        default:  return "Unknown Error";
    }
}

std::string HttpResponse::getHeadersAsString() const {
    std::ostringstream out;
    out << "HTTP/1.1 " << _status_code << " " << _status_message << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        out << it->first << ": " << it->second << "\r\n";
    }
    out << "\r\n"; // Blank line separating headers from body
    return out.str();
}

void HttpResponse::buildErrorResponse(int code, const std::map<int, std::string>& error_pages) {
    // 1. Set the basic status and headers
    setStatusCode(code);
    setHeader("Content-Type", "text/html");
    setHeader("Connection", "close");

    // 2. Try to load a custom error page from the configuration file
    std::map<int, std::string>::const_iterator it = error_pages.find(code);
    if (it != error_pages.end()) {
        std::ifstream file(it->second.c_str());
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            
            // setBody automatically handles setting the Content-Length header
            setBody(ss.str()); 
            return;
        }
        else {
            std::cerr << "[WARNING] Custom error page " << it->second << " not found. Using default." << std::endl;
        }
    }

    // 3. Fallback: Generate a default NGINX-style error page inline
    std::ostringstream html;
    html << "<html>\r\n"
         << "<head><title>" << _status_code << " " << _status_message << "</title></head>\r\n"
         << "<body bgcolor=\"white\">\r\n"
         << "<center><h1>" << _status_code << " " << _status_message << "</h1></center>\r\n"
         << "<hr><center>webserv/1.0</center>\r\n"
         << "</body>\r\n"
         << "</html>\r\n";
         
    setBody(html.str());
}

// ... (Keep your buildErrorResponse function from the previous step)