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
		
		// Getter methods for testing
		State getState() const { return state; }
		Method getMethod() const { return method; }
		std::string getPath() const { return path; }
		std::string getQueryString() const { return query_string; }
		std::string getHost() const { return Host; }
		bool foundHost() const { return found_host; }
		size_t getContentLength() const { return content_length; }
		std::map<std::string, std::string> getHeaders() const { return headers; }
		bool isHttpVersionValid() const { return http_version_valid; }
		std::string getTempFilename() const { return _temp_filename; }

		void startBodyParsing();
		int getErrorCode() const { return _error_code; }
		bool isChunked() const { return _is_chunked; }

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
		int _chunk_state;         // 0: Size, 1: Data, 2: Trailer (\r\n)
		size_t _chunk_bytes_left; // Bytes left to read in the current chunk
		bool _is_last_chunk;      // True when we read a 0 size
		
		void parseChunkedBody();
};
#endif