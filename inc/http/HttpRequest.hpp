#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <iostream>
#include <exception>
#include <map>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include "../utils/Helpers.hpp"

enum State
{
	READING_REQUEST_LINE,
	READING_HEADERS,
	HEADERS_COMPLETE,
	READING_BODY,
	COMPLETE,
	ERROR
};

enum Method
{
	GET,
	HEAD,
	POST,
	DELETE,
	UNKNOWN
};

class HttpRequest
{
	public:
		HttpRequest();
		~HttpRequest();
		void append(const char *buff, int size);
		void ShowBuff();
		State getState() const;
		Method getMethod() const;
		std::string getPath() const;
		std::string getQueryString() const;
		std::string getHost() const;
		bool foundHost() const;
		size_t getContentLength() const;
		std::map<std::string, std::string> getHeaders() const;
		bool isHttpVersionValid() const;
		std::string getTempFilename() const;

		void startBodyParsing();
		int getErrorCode() const;
		bool isChunked() const;

		std::string getCookie(const std::string& name) const;

	private:
		std::string buffer;
		State state;
		size_t content_length;
		size_t body_start_pos;
		bool http_version_valid;
		std::string Host;
		Method method;
		std::string path;
		std::string query_string;
		std::map <std::string, std::string> headers;
		size_t offset_;
		bool found_content_length;
		bool found_host;

		std::string _temp_filename;
		size_t _body_bytes_read;
		static int req_counter;
		int _error_code;

		void parse();
		void parseRequestLine();
		void parseHeaders();
		void loadMethod();
		void loadPathAndQuery();
		void checkHttpVersion();
		void loadHeaders(size_t start, size_t end);
		void setHeaderName(size_t start, size_t end, std::string &header_name);
		void setHeaderValue(size_t start, size_t end, std::string &header_value);
		bool _is_chunked;
		int _chunk_state;         
		size_t _chunk_bytes_left; 
		bool _is_last_chunk;      
		
		void parseChunkedBody();

		std::map<std::string, std::string> _cookies;
		void parseCookies(const std::string& cookie_header);
};
#endif