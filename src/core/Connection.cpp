#include "../../inc/core/Connection.hpp"
#include "../../inc/core/Post.hpp"

const char* Connection::ConnectionClosed::what() const throw() {
    return "Connection closed safely.";
}

void Connection::handlePost(const Location& loc, std::string _path) {
    // Failsafe check
    if (!_matched_server) {
        _response.buildErrorResponse(500, _possible_servers[0].error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }
    PostHandler post_handler(_request, _response, *_matched_server, loc);
    post_handler.execute(_path);

    // Finalize response flags for the Connection object
    _header_buffer = _response.getHeadersAsString();
    _is_response_ready = true;
}

void Connection::sendResponse()
{
    // PHASE 1: Send Headers
    if (_headers_sent < _header_buffer.length()) 
    {
        size_t bytes_left = _header_buffer.length() - _headers_sent;
        ssize_t sent = send(_client_fd, _header_buffer.c_str() + _headers_sent, bytes_left, MSG_NOSIGNAL | MSG_DONTWAIT);
        
        if (sent == -1) throw std::runtime_error("Error sending headers.");
		else if (sent == EWOULDBLOCK || sent == EAGAIN) {
			// Socket buffer is full, wait for the next EPOLLOUT event
			return;
		}
        
        _headers_sent += sent;
        updateActivity();
        return; // Return and wait for the next EPOLLOUT event
    }
	if (_request.getMethod() == HEAD) {
		// For HEAD requests, we only send headers, so we can close the connection after headers are sent.
		throw ConnectionClosed();
	}

    // PHASE 2: Send Body
    if (_response.isFile()) 
    {
        // It's a large file. Open-on-demand to avoid copy-constructor crashes!
        std::ifstream file(_response.getFilePath().c_str(), std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open response file.");

        // Jump to exactly where we left off
        file.seekg(_body_sent);

        // Read an 8KB chunk
        char chunk[8192];
        file.read(chunk, sizeof(chunk));
        size_t bytes_read = file.gcount();
        file.close();

        // Send the chunk over the socket
        if (bytes_read > 0) {
            ssize_t sent = send(_client_fd, chunk, bytes_read, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (sent == -1) throw std::runtime_error("Error sending file chunk.");
			else if (sent == EWOULDBLOCK || sent == EAGAIN) {
				// Socket buffer is full, wait for the next EPOLLOUT event
				return;
			}
            _body_sent += sent;
            updateActivity();
        }

        // Check if we are totally finished
        if (_body_sent >= _response.getFileSize()) {
			std::cout << "File fully sent. Closing connection." << "file is: " << _response.getFilePath() << std::endl;
            throw ConnectionClosed(); // Closes the socket safely
        }
    } 
    else 
    {
        // It's a small string body (like an error page HTML)
        const std::string& body = _response.getBody();
        size_t bytes_left = body.length() - _body_sent;
        ssize_t sent = send(_client_fd, body.c_str() + _body_sent, bytes_left, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (sent == -1) throw std::runtime_error("Error sending response body.");
		else if (sent == EWOULDBLOCK || sent == EAGAIN) {
			// Socket buffer is full, wait for the next EPOLLOUT event
			return;
		}
        _body_sent += sent;
        updateActivity();

        if (_body_sent >= body.length()) {
			std::cout << "Body fully sent. Closing connection." << std::endl;
            throw ConnectionClosed(); // Closes the socket safely
        }
    }
}
#include <sstream>
#include <cctype>

std::vector<std::string> Connection::buildCgiEnv(const std::string& physical_path) 
{
    std::vector<std::string> env;

    // 1. Standard CGI Metadata
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("SERVER_SOFTWARE=Webserv/1.0");

    // 2. Request Method
    std::string method_str;
    if (_request.getMethod() == GET) method_str = "GET";
    else if (_request.getMethod() == POST) method_str = "POST";
    else if (_request.getMethod() == DELETE) method_str = "DELETE";
    else if (_request.getMethod() == HEAD) method_str = "HEAD";
    else method_str = "UNKNOWN";
    
    env.push_back("REQUEST_METHOD=" + method_str);

    // 3. Path and Routing
  env.push_back("SCRIPT_FILENAME=" + physical_path);
    env.push_back("SCRIPT_NAME=" + _request.getPath());
    
    // The tester expects PATH_INFO to be the URI, and PATH_TRANSLATED to be the physical file!
    env.push_back("PATH_INFO=" + _request.getPath());
    env.push_back("PATH_TRANSLATED=" + physical_path);
    
    // The tester also strictly checks for this variable
    env.push_back("REQUEST_URI=" + _request.getPath());
    
    env.push_back("QUERY_STRING=" + _request.getQueryString());

    // 4. Server details (Fallbacks to defaults if server block is somehow empty)
    if (_matched_server && !_matched_server->server_names.empty()) {
        env.push_back("SERVER_NAME=" + _matched_server->server_names[0]);
    } else {
        env.push_back("SERVER_NAME=localhost");
    }

   struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    
    // getsockname looks at the _client_fd and fills local_addr with the server's IP and Port for this specific connection
    if (getsockname(_client_fd, (struct sockaddr*)&local_addr, &addr_len) == 0) {
        
        // Convert the port from network byte order to host integer
        int actual_port = ntohs(local_addr.sin_port); 
        
        std::ostringstream port_ss;
        port_ss << actual_port;
        env.push_back("SERVER_PORT=" + port_ss.str());
        
    } else {
        // Failsafe fallback just in case getsockname fails
        env.push_back("SERVER_PORT=8080"); 
    }

    // 5. Client HTTP Headers
    // The RFC requires we convert headers like "User-Agent: Mozilla" to "HTTP_USER_AGENT=Mozilla"
    std::map<std::string, std::string> headers = _request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) 
    {
        std::string key = it->first;
        
        // Capitalize and replace hyphens with underscores
        for (size_t i = 0; i < key.size(); ++i) {
            key[i] = (key[i] == '-') ? '_' : std::toupper(key[i]);
        }
        
        // Content-Type and Content-Length do NOT get the HTTP_ prefix
        if (key == "CONTENT_TYPE") {
            env.push_back("CONTENT_TYPE=" + it->second);
        } else if (key == "CONTENT_LENGTH") {
            env.push_back("CONTENT_LENGTH=" + it->second);
        } else {
            env.push_back("HTTP_" + key + "=" + it->second);
        }
    }

    // 6. Failsafe for Content-Length
    // If it wasn't in the headers but the request object has a size (e.g., chunked encoding)
    if (headers.find("content-length") == headers.end() && _request.getContentLength() > 0) 
    {
        std::ostringstream len_ss;
        len_ss << _request.getContentLength();
        env.push_back("CONTENT_LENGTH=" + len_ss.str());
    }

    // 7. Custom Webserver Variables (Very helpful for PHP file uploads)
    if (matched_location) {
        if (!matched_location->upload_dir.empty()) {
            env.push_back("UPLOAD_DIR=" + matched_location->upload_dir);
        }
        if (!matched_location->root.empty()) {
            env.push_back("DOCUMENT_ROOT=" + matched_location->root);
        }
    }

    return env;
}

int Connection::checkCGI(const std::string& path) {
	// Check if the resolved path matches any CGI location
	size_t ext_pos = path.find_last_of('.');
	if (ext_pos == std::string::npos)
		return 0; // No extension, so not a CGI script
	std::string ext = path.substr(ext_pos);
	std::map<std::string, std::string>::const_iterator it = matched_location->cgi_pass.find(ext);
	if (it == matched_location->cgi_pass.end())
		return 0; // Extension not configured for CGI
	_cgi = CgiHandler(path, it->second, _request.getMethod(), _request.getTempFilename());
	std::vector<std::string> envp_vec = buildCgiEnv(path);
	if (!_cgi.execute(envp_vec)) {
		_response.buildErrorResponse(500, _matched_server->error_pages);
		_header_buffer = _response.getHeadersAsString();
		_is_response_ready = true;
		return 0; // Failed to execute CGI, but we handled it gracefully with an error response
	}
	return 1; // Extension is configured for CGI
}

#include <cstdlib> // For std::atoi

void Connection::readCgiOutput()
{
	
    if (!_cgi.readOutputNonBlocking())
	{
        
        std::string raw = _cgi.getOutput();
        size_t header_end = raw.find("\r\n\r\n");
        size_t sep_len = 4;
        
        // Fallback for single line breaks
        if (header_end == std::string::npos) {
            header_end = raw.find("\n\n");
            sep_len = 2;
        }

        if (header_end != std::string::npos) 
        {
            std::string headers_part = raw.substr(0, header_end);
            std::string body_part = raw.substr(header_end + sep_len);

            // Parse the headers from the CGI script
            size_t pos = 0;
            while (pos < headers_part.length()) {
                size_t eol = headers_part.find('\n', pos);
                if (eol == std::string::npos) eol = headers_part.length();
                
                std::string line = headers_part.substr(pos, eol - pos);
                if (!line.empty() && line[line.length() - 1] == '\r') {
                    line.erase(line.length() - 1);
                }
                pos = eol + 1;

                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string val = line.substr(colon + 1);
                    
                    // Trim leading spaces
                    size_t start = val.find_first_not_of(" \t");
                    if (start != std::string::npos) val = val.substr(start);

                    if (key == "Status") {
                        _response.setStatusCode(std::atoi(val.c_str()));
                    } else {
                        _response.setHeader(key, val);
                    }
                }
            }
            _response.setBody(body_part);
        } 
        else 
        {
            // No headers found, treat entire string as body
            _response.setBody(raw);
        }

		_header_buffer = _response.getHeadersAsString();
		_is_response_ready = true;
	}
	updateActivity();
}

int Connection::handleRequest()
{
	char buff[1024];
	ssize_t bread;

	bread = recv(_client_fd, buff, sizeof(buff), 0);
	if (bread == 0)
		throw ConnectionClosed();
	if (bread == -1)
		throw std::runtime_error("Error: recv failed.");

	updateActivity();
	
	_request.append(buff, bread);

	if (_request.foundHost() && _matched_server == NULL)
	{
		_matched_server = findCorrectServer(_request.getHost());
		matched_location = findLocation(_matched_server, _request.getPath());
	}
	if (_request.getState() == ERROR)
	{
		std::map<int, std::string> err_pages;
		if (_matched_server)
			err_pages = _matched_server->error_pages;
		else
			err_pages = _possible_servers[0].error_pages;

		_response.buildErrorResponse(_request.getErrorCode(), err_pages);
		_header_buffer = _response.getHeadersAsString();
		_is_response_ready = true;
		
		return 0; // STOP EXECUTION! Do not parse headers or bodies.
	}
	if (_request.getState() == HEADERS_COMPLETE)
	{
		size_t current_limit = _matched_server->client_max_body_size;
		if (matched_location != NULL) {
			current_limit = matched_location->client_max_body_size;
		}
		if (_request.isChunked() == false && _request.getContentLength() > current_limit)
		{
			_response.buildErrorResponse(413, _matched_server->error_pages);
			_header_buffer = _response.getHeadersAsString();
			_is_response_ready = true;
			return 0;
		}
		_request.startBodyParsing();
	}
	if (_request.getState() == COMPLETE) 
    {
		std::string req_method_str;
		if (_request.getMethod() == GET) req_method_str = "GET";
		else if (_request.getMethod() == POST) req_method_str = "POST";
		else if (_request.getMethod() == DELETE) req_method_str = "DELETE";
		else if (_request.getMethod() == HEAD) req_method_str = "HEAD";
		else req_method_str = "UNKNOWN";

		bool is_allowed = false;
		for (size_t i = 0; i < matched_location->allowed_methods.size(); ++i) {
			if (matched_location->allowed_methods[i] == req_method_str) {
				is_allowed = true;
				break;
			}
		}

		if (req_method_str == "HEAD")
			req_method_str = "GET"; // Handle HEAD as GET internally, but we'll just send headers later without body.
		if (!is_allowed) {
			_response.buildErrorResponse(405, _matched_server->error_pages);
			_header_buffer = _response.getHeadersAsString();
			_is_response_ready = true;
			return 0; // Stop execution, the method is forbidden here!
		}
		std::string path = resolvePhysicalPath(_request.getPath(), *matched_location);
		if (checkCGI(path) == 1)
			return 1;
        if (_request.getMethod() == GET) {
            handleGet(*matched_location, path);
        } else if (_request.getMethod() == POST) {
            handlePost(*matched_location, path);
        } else if (_request.getMethod() == DELETE) {
            handleDelete(path);
        }
    }
	return 0;
}

const Server *Connection::findCorrectServer(const std::string &host)
{
	// 1. Clean the Host header by removing the port if it exists
	// (e.g., "localhost:8080" becomes "localhost")
	std::string hostname = host;
	size_t colon_pos = hostname.find(':');
	if (colon_pos != std::string::npos)
	{
		hostname = hostname.substr(0, colon_pos);
	}

	// 2. Iterate through all possible server blocks mapped to this socket
	for (size_t i = 0; i < _possible_servers.size(); ++i)
	{

		// Check if the cleaned Host header matches any of the server_names
		for (size_t j = 0; j < _possible_servers[i].server_names.size(); ++j)
		{
			if (_possible_servers[i].server_names[j] == hostname)
			{
				// Exact match found!
				return &_possible_servers[i];
			}
		}
	}

	// 3. Nginx Default Behavior: If the Host header doesn't match any server_name
	// (or if it's an IP address), default to the first server block defined for this IP:Port.
	return &_possible_servers[0];
}

const Location *Connection::findLocation(const Server *server, const std::string &path)
{

	const Location *best_match = NULL;
	size_t max_match_length = 0;

	// Iterate through all location blocks in the matched server
	for (size_t i = 0; i < server->locations.size(); ++i)
	{
		const std::string &loc_path = server->locations[i].path;

		// Check if the requested path starts with the location's path (Prefix matching)
		if (path.find(loc_path) == 0)
		{

			// Longest Prefix Match logic:
			// If requested path is "/images/cat.jpg", and we have locations "/" and "/images/",
			// we want to match "/images/" because it is more specific.
			if (loc_path.length() > max_match_length)
			{

				// Additional safety: ensure it's a clean boundary match.
				// For example, location "/img" shouldn't match path "/images/cat.jpg".
				// It should either be an exact match, end with a trailing slash,
				// or the next character in the requested path must be a slash.
				bool is_clean_boundary = false;

				if (path.length() == loc_path.length())
				{
					is_clean_boundary = true; // Exact match (e.g., loc: "/test", path: "/test")
				}
				else if (loc_path[loc_path.length() - 1] == '/')
				{
					is_clean_boundary = true; // Ends with slash (e.g., loc: "/images/", path: "/images/cat.jpg")
				}
				else if (path[loc_path.length()] == '/')
				{
					is_clean_boundary = true; // Next char is slash (e.g., loc: "/images", path: "/images/cat.jpg")
				}

				if (is_clean_boundary)
				{
					max_match_length = loc_path.length();
					best_match = &server->locations[i];
				}
			}
		}
	}

	// Returns a pointer to the best matching location block, or NULL if none matched
	// (though usually you always have a fallback "/" location defined in the config)
	return best_match;
}