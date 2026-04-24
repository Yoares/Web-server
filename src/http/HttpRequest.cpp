#include "../../inc/http/HttpRequest.hpp"

int HttpRequest::req_counter = 0;

HttpRequest::HttpRequest()
{
	this->buffer = "";
	this->state = READING_REQUEST_LINE;
	this->offset_ = 0;
	this->content_length = 0;
	this->body_start_pos = 0;
	this->http_version_valid = false;
	this->method = UNKNOWN;
	this->path = "";
	this->query_string = "";
	this->headers.clear();
	this->found_content_length = false;
	this->found_host = false;
	this->_body_bytes_read = 0;
	this->_temp_filename = "/tmp/webserv_body_" + to_string(req_counter) + ".tmp";
	req_counter++;
	this->_error_code = 200;
}

HttpRequest::~HttpRequest()
{
	std::remove(_temp_filename.c_str());
}
void HttpRequest::loadMethod()
{
	if (buffer.find("GET") == 0)
	{
		method = GET;
		offset_ += 3;
	}
	else if (buffer.find("POST") == 0)
	{
		method = POST;
		offset_ += 4;
	}
	else if (buffer.find("DELETE") == 0)
	{
		method = DELETE;
		offset_ += 6;
	}
	else
		method = UNKNOWN;
}

void HttpRequest::setHeaderName(size_t start, size_t end, std::string &header_name)
{
	header_name = buffer.substr(start, end - start);
	if (header_name.empty())
	{
		state = ERROR;
		_error_code = 400; // Bad Request
		headers.clear();
		std::cout << "Header parsing error: Empty header name." << std::endl;
		return;
	}
	for (size_t i = 0; i < header_name.length(); i++)
	{
		if (header_name[i] >= 127 || header_name[i] <= 32)
		{
			state = ERROR;
			_error_code = 400;
			headers.clear();
			std::cout << "Header parsing error: Invalid character in header name. \'" << header_name[i] << "\'" << std::endl;
			return;
		}
		header_name[i] = std::tolower(header_name[i]);
	}
}

void HttpRequest::setHeaderValue(size_t start, size_t end, std::string &header_value)
{
	header_value = buffer.substr(start, end - start);
	while (header_value.length() > 0 && (header_value[0] == ' ' || header_value[0] == '\t'))
		header_value.erase(0, 1);
	while (header_value.length() > 0 && (header_value[header_value.length() - 1] == ' ' || header_value[header_value.length() - 1] == '\t'))
		header_value.erase(header_value.length() - 1, 1);
	for (size_t i = 0; i < header_value.length(); i++)
	{
		if (header_value[i] >= 127 || (header_value[i] <= 31 && header_value[i] != '\t'))
		{
			state = ERROR;
			_error_code = 400;
			headers.clear();
			std::cout << "Header parsing error: Invalid character in header value." << std::endl;
			return;
		}
	}
}

void HttpRequest::loadHeaders(size_t start, size_t end)
{
	size_t i = start;
	while (i < end)
	{
		size_t header_start = i;
		size_t header_end = buffer.find("\r\n", i);

		size_t colon_pos = buffer.find(':', header_start);
		if (colon_pos == std::string::npos || colon_pos < header_start)
		{
			state = ERROR;
			_error_code = 400;
			headers.clear();
			std::cout << "Header parsing error: No colon found in header line." << std::endl;
			return;
		}
		std::string header_name;
		setHeaderName(header_start, colon_pos, header_name);
		if (state == ERROR)
			return;
		std::string header_value;
		setHeaderValue(colon_pos + 1, header_end, header_value);
		if (state == ERROR)
			return;
		headers[header_name] = header_value;
		if (header_name == "host")
		{
			Host = header_value;
			found_host = true;
		}
		else if (header_name == "content-length")
		{
			content_length = std::atol(header_value.c_str());
			found_content_length = true;
		}
		i = header_end + 2;
	}
}

void HttpRequest::loadPathAndQuery()
{
	size_t path_end = buffer.find(' ', offset_);
	size_t query_pos = buffer.find('?', offset_);
	if (query_pos != std::string::npos && path_end != std::string::npos && query_pos < path_end)
	{
		query_string = buffer.substr(query_pos + 1, path_end - query_pos - 1);
		path_end = query_pos;
	}
	if (path_end == std::string::npos)
	{
		http_version_valid = false;
		state = ERROR;
		_error_code = 400;
		return;
	}
	if (query_pos == std::string::npos)
	{
		query_string = "";
	}
	path = buffer.substr(offset_, path_end - offset_);
	offset_ = path_end + 1;
}

void HttpRequest::checkHttpVersion()
{
	if (buffer.find("HTTP/1.1", offset_) != std::string::npos)
		http_version_valid = true;
	else
	{
		http_version_valid = false;
		state = ERROR;
		_error_code = 505;
	}
}

void HttpRequest::parseRequestLine()
{
	size_t pos = buffer.find("\r\n");
	if (pos != std::string::npos)
	{
		loadMethod();
		if (method == UNKNOWN)
		{
			state = ERROR;
			_error_code = 501;
			return;
		}
		if (buffer[offset_] != ' ')
		{
			method = UNKNOWN;
			state = ERROR;
			_error_code = 400;
			return;
		}
		offset_++;

		if (buffer[offset_] == '/')
		{
			loadPathAndQuery();
			if (state == ERROR)
				return;
		}
		else
		{
			path = "ERROR";
			state = ERROR;
			_error_code = 400;
			return;
		}
		checkHttpVersion();
		if (http_version_valid == false)
			return;
		state = READING_HEADERS;
		offset_ = pos + 2;
	}
}

void HttpRequest::parseHeaders()
{
	size_t pos = buffer.find("\r\n\r\n");
	if (pos != std::string::npos)
	{
		loadHeaders(offset_, pos - 1);
		if (state == ERROR)
			return;
		if (found_host == false)
		{
			state = ERROR;
			_error_code = 400;
			return;
		}
		if (method == POST && found_content_length == false)
		{
			state = ERROR;
			_error_code = 411;
			std::cerr << "[ERROR] 411 Length Required (Chunked Encoding not supported)" << std::endl;
			return;
		}
		if (method == GET || method == DELETE)
		{
			state = COMPLETE;
			return;
		}
		state = HEADERS_COMPLETE;
		body_start_pos = pos + 4;
	}
}

void HttpRequest::parse()
{
	if (state == READING_REQUEST_LINE)
	{
		parseRequestLine();
	}
	if (state == READING_HEADERS)
		parseHeaders();
	if (state == ERROR)
	{
		std::cerr << "Error parsing request. Current buffer content:" << std::endl;
		std::cerr << "---" << std::endl;
		std::cerr << buffer;
		std::cerr << "---" << std::endl;
	}
	if (state == COMPLETE)
	{
		std::cout << "Request parsing complete. Final buffer content:" << std::endl;
		std::cout << "---" << std::endl;
		std::cout << buffer;
		std::cout << "---" << std::endl;
	}
}
void HttpRequest::startBodyParsing()
{
	if (state != HEADERS_COMPLETE)
		return ;
	state = READING_BODY;

	// Write any leftover bytes caught in the header buffer
	if (buffer.size() > body_start_pos)
	{
		size_t leftover_len = buffer.size() - body_start_pos;

		if (_body_bytes_read + leftover_len > content_length)
		{
			leftover_len = content_length - _body_bytes_read;
		}

		std::ofstream file(_temp_filename.c_str(), std::ios::binary | std::ios::app);
		if (file.is_open()) {
			file.write(buffer.c_str() + body_start_pos, leftover_len);
			file.close();
		} else {
			_error_code = 500;
			state = ERROR;
			std::cerr << "Failed to open temp file for body stream." << std::endl;
			return;
		}
		_body_bytes_read += leftover_len;
	}

	// Clear the body from RAM buffer to save memory
	buffer.erase(body_start_pos);

	if (_body_bytes_read >= content_length)
		state = COMPLETE;
}

void HttpRequest::append(const char *buff, int size)
{
	if (state == ERROR || state == COMPLETE)
		return;
	if (state != READING_BODY)
	{
		buffer.append(buff, size);
		if (buffer.size() > 8192 && state != HEADERS_COMPLETE) {
			state = ERROR;
			_error_code = 431;
			std::cerr << "[ERROR] 431 Request Header Fields Too Large" << std::endl;
			return;
		}
		parse();
	}
	else
	{
		// Strictly in body phase, stream directly to disk
		size_t bytes_to_write = size;
		
		if (_body_bytes_read + bytes_to_write > content_length) {
			bytes_to_write = content_length - _body_bytes_read;
		}

		std::ofstream file(_temp_filename.c_str(), std::ios::binary | std::ios::app);
		if (file.is_open()) {
			file.write(buff, bytes_to_write);
			file.close();
			_body_bytes_read += bytes_to_write;
		}
		else {
			state = ERROR;
			_error_code = 500;
			std::cerr << "Failed to open temp file for body stream." << std::endl;
			return;
		}

		if (_body_bytes_read >= content_length) {
			state = COMPLETE;
			std::cout << "[INFO] Successfully streamed large body to " << _temp_filename << std::endl;
		}
	}
}

void HttpRequest::ShowBuff()
{
	std::cout << buffer;
}