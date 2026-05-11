#include "../../inc/core/Connection.hpp"
#include "../../inc/core/Post.hpp"

const char* Connection::ConnectionClosed::what() const throw() {
    return "Connection closed safely.";
}

void Connection::handlePost(const Location& loc) {
    // Failsafe check
    if (!_matched_server) {
        _response.buildErrorResponse(500, _possible_servers[0].error_pages);
        _header_buffer = _response.getHeadersAsString();
        _is_response_ready = true;
        return;
    }

    // Instantiate handler and execute logic
    PostHandler post_handler(_request, _response, *_matched_server, loc);
    post_handler.execute();

    // Finalize response flags for the Connection object
    _header_buffer = _response.getHeadersAsString();
    _is_response_ready = true;
}

<<<<<<< HEAD
=======
// void Connection::handleDelete(const Location& loc) {
//   //
//   (void)loc; // Avoid unused parameter warning
// }
>>>>>>> 5e6f155 (refactor HttpRequest and Connection classes to improve host handling and body parsing)

void Connection::sendResponse()
{
    // PHASE 1: Send Headers
    if (_headers_sent < _header_buffer.length()) 
    {
        size_t bytes_left = _header_buffer.length() - _headers_sent;
        ssize_t sent = send(_client_fd, _header_buffer.c_str() + _headers_sent, bytes_left, 0);
        
        if (sent == -1) throw std::runtime_error("Error sending headers.");
        
        _headers_sent += sent;
        updateActivity();
        return; // Return and wait for the next EPOLLOUT event
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
            ssize_t sent = send(_client_fd, chunk, bytes_read, 0);
            if (sent == -1) throw std::runtime_error("Error sending file chunk.");
            _body_sent += sent;
            updateActivity();
        }

        // Check if we are totally finished
        if (_body_sent >= _response.getFileSize()) {
            throw ConnectionClosed(); // Closes the socket safely
        }
    } 
    else 
    {
        // It's a small string body (like an error page HTML)
        const std::string& body = _response.getBody();
        size_t bytes_left = body.length() - _body_sent;
        
        ssize_t sent = send(_client_fd, body.c_str() + _body_sent, bytes_left, 0);
        if (sent == -1) throw std::runtime_error("Error sending string body.");
        
        _body_sent += sent;
        updateActivity();

        if (_body_sent >= body.length()) {
            throw ConnectionClosed(); // Closes the socket safely
        }
    }
}

void Connection::handleRequest()
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
		
		return; // STOP EXECUTION! Do not parse headers or bodies.
	}
	if (_request.getState() == HEADERS_COMPLETE)
	{
			// Check server-level client_max_body_size
		if (_request.getContentLength() > _matched_server->client_max_body_size)
		{
			_response.buildErrorResponse(413, _matched_server->error_pages);
			_header_buffer = _response.getHeadersAsString();
			_is_response_ready = true;
			return; // STOP EXECUTION! Do not parse body.
		}
		// Resume parsing the body (opens file and writes leftover buffer)
		_request.startBodyParsing();
	}
	if (_request.getState() == COMPLETE) 
    {
        if (_request.getMethod() == GET) {
            handleGet(*matched_location);
        } else if (_request.getMethod() == POST) {
            handlePost(*matched_location);
        } else if (_request.getMethod() == DELETE) {
            handleDelete(*matched_location);
        }
    }
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