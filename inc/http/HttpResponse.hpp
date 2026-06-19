#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include "../config/Config.hpp"
#include "../utils/Helpers.hpp"

class HttpResponse
{
	private:
		int _status_code;
		std::string _status_message;
		std::map<std::string, std::string> _headers;

		std::string _body;

		bool _is_file;
		std::string _file_path;
		size_t _file_size;

		std::string getReasonPhrase(int code) const;

		std::vector<std::string> _set_cookies;

	public:
		HttpResponse();
		~HttpResponse();

		void setStatusCode(int code);
		void setHeader(const std::string &key, const std::string &value);
		void setBody(const std::string &body);

		void setFile(const std::string &path, size_t size);

		void buildErrorResponse(int code, const std::map<int, std::string> &error_pages);
		std::string getHeadersAsString() const;

		bool isFile() const;
		std::string getFilePath() const;
		size_t getFileSize() const;
		std::string getBody() const;

		void setCookie(const std::string &name, const std::string &value, int max_age);
};

#endif