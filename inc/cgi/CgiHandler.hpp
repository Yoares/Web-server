#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <sys/types.h>
#include <ctime>
#include "../http/HttpRequest.hpp"

enum CgiState
{
	CGI_INIT,
	CGI_RUNNING,
	CGI_COMPLETE,
	CGI_ERROR
};

enum CgiOutputState
{
	OUTPUT_READING_HEADERS,
	OUTPUT_READING_BODY,
	OUTPUT_DONE
};

class CgiHandler
{
	public:
		CgiHandler(const std::string &script_path,
				const std::string &cgi_bin,
				int method,
				const std::string &tmp_post_file);

		CgiHandler();
		CgiHandler &operator=(const CgiHandler &other);
		~CgiHandler();

		bool execute(const std::vector<std::string> &envp_vec);
		bool readOutputNonBlocking();
		void killProcess();
		bool checkTimeout(int timeout_seconds = 5);

		pid_t getPid() const;
		int getStdoutFd() const;
		CgiState getState() const;
		int getMethod() const;
		std::string getOutFile() const;
		size_t getBodySize() const { return body_size; }

		std::string getOutput() const;
		int getExitStatus() const;

	private:
		pid_t _pid;
		int _method;
		CgiState _state;
		CgiOutputState _output_state;

		int _stdout_pipe[2];
		int cgi_output_fd;

		std::string _script_path;
		std::string _cgi_bin;
		std::string _tmp_post_file;
		std::string _output_file;
		size_t body_size;

		std::string _output_buffer;
		int _exit_status;
		time_t _start_time;

		char **_vecToCharArray(const std::vector<std::string> &vec) const;
		void _freeCharArray(char **arr) const;
};

#endif