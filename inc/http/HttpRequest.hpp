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
		size_t getContentLength() const { return content_length; }
		std::map<std::string, std::string> getHeaders() const { return headers; }
		bool isHttpVersionValid() const { return http_version_valid; }
		

		void startBodyParsing();
		void setUploadDir(const std::string& dir) { _upload_dir = dir; }

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
		std::string _upload_dir;
		static int req_counter;

		void parse();
		void parseRequestLine();
		void parseHeaders();
		void loadMethod();
		void loadPathAndQuery();
		void checkHttpVersion();
		void loadHeaders(size_t start, size_t end);
		void setHeaderName(size_t start, size_t end, std::string &header_name);
		void setHeaderValue(size_t start, size_t end, std::string &header_value);
};
#endif