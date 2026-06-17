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
	this->Host = "";
	this->query_string = "";
	this->headers.clear();
	this->found_content_length = false;
	this->found_host = false;
	this->_body_bytes_read = 0;
	this->_temp_filename = "/tmp/webserv_body_" + to_string(req_counter) + ".tmp";
	req_counter++;
	this->_error_code = 200;
	_is_chunked = false;
	_chunk_state = 0;
	_chunk_bytes_left = 0;
	_is_last_chunk = false;
}

void HttpRequest::parseChunkedBody()
{
	while (!buffer.empty() && state == READING_BODY)
	{
		if (_chunk_state == 0)
		{
			size_t pos = buffer.find("\r\n");
			if (pos == std::string::npos)
			{
				if (buffer.length() > 100)
				{
					_error_code = 400;
					state = ERROR;
				}
				return;
			}

			std::string hex_str = buffer.substr(0, pos);
			size_t semi_pos = hex_str.find(';');
			if (semi_pos != std::string::npos)
				hex_str = hex_str.substr(0, semi_pos);
			char *end;
			long val = std::strtol(hex_str.c_str(), &end, 16);
			if (end == hex_str.c_str() || (*end != '\0' && *end != ' ' && *end != '\t'))
			{
				_error_code = 400;
				state = ERROR;
				return;
			}
			_chunk_bytes_left = val;
			if (_chunk_bytes_left == 0)
				_is_last_chunk = true;
			buffer.erase(0, pos + 2);
			_chunk_state = (_chunk_bytes_left == 0) ? 2 : 1;
		}
		else if (_chunk_state == 1)
		{
			size_t to_write = std::min(buffer.length(), _chunk_bytes_left);
			if (to_write > 0)
			{
				std::ofstream file(_temp_filename.c_str(), std::ios::binary | std::ios::app);
				if (!file.is_open())
				{
					_error_code = 500;
					state = ERROR;
					return;
				}
				file.write(buffer.c_str(), to_write);
				file.close();
				buffer.erase(0, to_write);
				_chunk_bytes_left -= to_write;
				content_length += to_write;
			}
			if (_chunk_bytes_left == 0)
				_chunk_state = 2;
			else
				return;
		}
		else if (_chunk_state == 2)
		{
			if (buffer.length() < 2)
				return;
			if (buffer[0] == '\r' && buffer[1] == '\n')
			{
				buffer.erase(0, 2);
				if (_is_last_chunk)
				{
					state = COMPLETE;
					return;
				}
				else
					_chunk_state = 0;
			}
			else
			{
				_error_code = 400;
				state = ERROR;
				return;
			}
		}
	}
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
	else if (buffer.find("HEAD") == 0)
	{
		method = HEAD;
		offset_ += 4;
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
		_error_code = 400;
		headers.clear();
		return;
	}
	for (size_t i = 0; i < header_name.length(); i++)
	{
		if (header_name[i] >= 127 || header_name[i] <= 32)
		{
			state = ERROR;
			_error_code = 400;
			headers.clear();
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
			if (found_host)
			{
				state = ERROR;
				_error_code = 400;
				return;
			}
			Host = header_value;
			found_host = true;
		}
		else if (header_name == "content-length")
		{
			if (found_content_length || _is_chunked)
			{
				state = ERROR;
				_error_code = 400;
				return;
			}
			char *endp;
			content_length = std::strtol(header_value.c_str(), &endp, 10);
			if (*endp != '\0' || errno == ERANGE || content_length < 0 || endp == header_value.c_str())
			{
				state = ERROR;
				_error_code = 400;
				return;
			}
			found_content_length = true;
		}
		else if (header_name == "cookie")
		{
			parseCookies(header_value.c_str());
		}
		else if (header_name == "transfer-encoding" && header_value == "chunked")
		{
			_is_chunked = true;
			if (found_content_length)
			{
				state = ERROR;
				_error_code = 400;
				return;
			}
		}
		else if (header_name == "transfer-encoding" && header_value != "chunked")
		{
			state = ERROR;
			_error_code = 501;
			return;
		}
		i = header_end + 2;
	}
}

void HttpRequest::loadPathAndQuery()
{
	size_t line_end = buffer.find("\r\n", offset_);
	size_t path_end = buffer.find('?', offset_);
	if (path_end == std::string::npos || path_end > line_end)
	{
		path_end = buffer.find(' ', offset_);
		if (path_end == std::string::npos || path_end > line_end)
		{
			state = ERROR;
			_error_code = 400;
			return;
		}
		path = buffer.substr(offset_, path_end - offset_);
		query_string = "";
		offset_ = path_end + 1;
	}
	else
	{
		path = buffer.substr(offset_, path_end - offset_);
		size_t query_end = buffer.find(' ', path_end);
		if (query_end == std::string::npos || query_end > line_end)
		{
			state = ERROR;
			_error_code = 400;
			return;
		}
		query_string = buffer.substr(path_end + 1, query_end - path_end - 1);
		offset_ = query_end + 1;
	}
}

void HttpRequest::checkHttpVersion()
{
	if (buffer.length() >= offset_ + 8 && buffer.substr(offset_, 8) == "HTTP/1.1")
	{
		http_version_valid = true;
	}
	else
	{
		http_version_valid = false;
		state = ERROR;
		if (buffer.length() >= offset_ + 5 && buffer.substr(offset_, 5) == "HTTP/")
			_error_code = 505;
		else
			_error_code = 400;
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
		if (_is_chunked && found_content_length)
		{
			found_content_length = false;
			content_length = 0;
		}
		if (method == POST && found_content_length == false && _is_chunked == false)
		{
			state = ERROR;
			_error_code = 411;
			return;
		}
		if (method == GET || method == DELETE || method == HEAD)
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
	if (state == HEADERS_COMPLETE && method == POST && content_length == 0 && !_is_chunked)
		state = COMPLETE;
}

void HttpRequest::startBodyParsing()
{
	state = READING_BODY;

	buffer.erase(0, body_start_pos);
	if (_is_chunked)
		parseChunkedBody();
	else
	{
		size_t leftover_len = buffer.size();
		if (_body_bytes_read + leftover_len > content_length)
			leftover_len = content_length - _body_bytes_read;
		std::ofstream file(_temp_filename.c_str(), std::ios::binary | std::ios::app);
		if (file.is_open())
		{
			file.write(buffer.c_str(), leftover_len);
			file.close();
		}
		else
		{
			_error_code = 500;
			state = ERROR;
			return;
		}
		_body_bytes_read += leftover_len;
		buffer.erase(0, leftover_len);
		if (_body_bytes_read >= content_length)
			state = COMPLETE;
	}
}

void HttpRequest::append(const char *buff, int size)
{
	if (state == ERROR || state == COMPLETE)
		return;
	if (state != READING_BODY)
	{
		buffer.append(buff, size);
		if (buffer.size() > 8192 && state != HEADERS_COMPLETE)
		{
			if (state == READING_REQUEST_LINE)
				_error_code = 414;
			else
				_error_code = 431;
			state = ERROR;
			return;
		}
		parse();
	}
	else
	{
		if (_is_chunked)
		{
			buffer.append(buff, size);
			parseChunkedBody();
		}
		else
		{
			size_t bytes_to_write = size;
			if (_body_bytes_read + bytes_to_write > content_length)
				bytes_to_write = content_length - _body_bytes_read;
			std::ofstream file(_temp_filename.c_str(), std::ios::binary | std::ios::app);
			if (file.is_open())
			{
				file.write(buff, bytes_to_write);
				file.close();
				_body_bytes_read += bytes_to_write;
			}
			else
			{
				state = ERROR;
				_error_code = 500;
				std::cerr << "Failed to open temp file for body stream." << std::endl;
				return;
			}
			if (_body_bytes_read >= content_length)
			{
				state = COMPLETE;
				std::cout << "[INFO] Successfully streamed large body to " << _temp_filename << std::endl;
			}
		}
	}
}

void HttpRequest::ShowBuff()
{
	std::cout << buffer;
}

void HttpRequest::parseCookies(const std::string &cookie_header)
{
	size_t pos = 0;
	while (pos < cookie_header.length())
	{
		size_t semi_pos = cookie_header.find(';', pos);
		if (semi_pos == std::string::npos)
			semi_pos = cookie_header.length();
		size_t eq_pos = cookie_header.find('=', pos);
		if (eq_pos != std::string::npos && eq_pos < semi_pos)
		{
			std::string key = cookie_header.substr(pos, eq_pos - pos);
			std::string value = cookie_header.substr(eq_pos + 1, semi_pos - eq_pos - 1);
			size_t key_start = key.find_first_not_of(" \t");
			if (key_start != std::string::npos)
				key = key.substr(key_start);
			_cookies[key] = value;
		}
		pos = semi_pos + 1;
	}
}

std::string HttpRequest::getCookie(const std::string &name) const
{

	std::map<std::string, std::string>::const_iterator it = _cookies.find(name);
	if (it != _cookies.end())
	{
		return it->second;
	}
	return "";
}