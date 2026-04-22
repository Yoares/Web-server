#include "../../inc/core/Connection.hpp"

void Connection::handleRequest()
{
	char buff[1024];
	ssize_t bread;

	bread = recv(_client_fd, buff, sizeof(buff), 0);
	if (bread == 0)
	{
		throw ConnectionClosed();
		return;
	}
	if (bread == -1)
	{
		throw std::runtime_error("Error: recv failed.");
	}
	_request.append(buff, bread);
	if (_request.getState() == HEADERS_COMPLETE)
	{
		_matched_server = findCorrectServer(_request.getHost());

		if (_matched_server)
		{
			// Check server-level client_max_body_size
			if (_request.getContentLength() > _matched_server->client_max_body_size)
			{
				std::cerr << "[ERROR] 413 Payload Too Large" << std::endl;
				// TODO: Send 413 error response to client, then throw or return
			}

			const Location *matched_location = findLocation(_matched_server, _request.getPath());

			if (matched_location && matched_location->upload_enable)
			{
				_request.setUploadDir(matched_location->upload_dir);
			}
		}

		// Resume parsing the body (opens file and writes leftover buffer)
		_request.startBodyParsing();
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