#include "../../inc/cgi/CgiHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

CgiHandler::CgiHandler(const std::string &script_path, const std::string &cgi_bin, int method, const std::string &tmp_post_file)
	: _pid(-1), _method(method), _state(CGI_INIT),
	  _script_path(script_path), _cgi_bin(cgi_bin), _tmp_post_file(tmp_post_file),
	  _exit_status(0)
{
	_stdout_pipe[0] = -1;
	_stdout_pipe[1] = -1;
	cgi_output_fd = -1;
	_output_state = OUTPUT_READING_HEADERS;
	body_size = 0;
	_output_file = "";
}

CgiHandler::~CgiHandler()
{
	if (_stdout_pipe[0] != -1)
		close(_stdout_pipe[0]);
	if (_stdout_pipe[1] != -1)
		close(_stdout_pipe[1]);
	if (cgi_output_fd != -1)
		close(cgi_output_fd);

	if (_pid > 0)
	{
		killProcess();
		waitpid(_pid, NULL, 0);
	}
}

bool CgiHandler::execute(const std::vector<std::string> &envp_vec)
{
	if (pipe(_stdout_pipe) == -1)
	{
		_state = CGI_ERROR;
		return false;
	}
	char resolved_script[4096];
	if (realpath(_script_path.c_str(), resolved_script) != NULL)
		_script_path = resolved_script;
	char resolved_bin[4096];
	if (realpath(_cgi_bin.c_str(), resolved_bin) != NULL)
		_cgi_bin = resolved_bin;
	fcntl(_stdout_pipe[0], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
	_start_time = std::time(NULL);
	_pid = fork();
	if (_pid == -1)
	{
		_state = CGI_ERROR;
		return false;
	}
	if (_pid == 0)
	{
		if (_method == POST)
		{
			int file_fd = open(_tmp_post_file.c_str(), O_RDONLY);
			if (file_fd != -1)
			{
				dup2(file_fd, STDIN_FILENO);
				close(file_fd);
			}
		}
		else
		{
			int dev_null = open("/dev/null", O_RDONLY);
			if (dev_null != -1)
			{
				dup2(dev_null, STDIN_FILENO);
				close(dev_null);
			}
		}
		dup2(_stdout_pipe[1], STDOUT_FILENO);
		close(_stdout_pipe[0]);
		close(_stdout_pipe[1]);
		std::vector<std::string> args;
		args.push_back(_cgi_bin);
		args.push_back(_script_path);
		char **argv = _vecToCharArray(args);
		char **envp = _vecToCharArray(envp_vec);
		std::string script_dir = _script_path.substr(0, _script_path.find_last_of('/'));
		if (!script_dir.empty())
			chdir(script_dir.c_str());
		execve(_cgi_bin.c_str(), argv, envp);
		exit(1);
	}
	close(_stdout_pipe[1]);
	_stdout_pipe[1] = -1;
	_state = CGI_RUNNING;
	return true;
}

std::string CgiHandler::getOutFile() const
{
	return _output_file;
}

bool CgiHandler::readOutputNonBlocking()
{
	if (_state != CGI_RUNNING)
		return false;

	char buffer[4096];
	ssize_t bytes_read = read(_stdout_pipe[0], buffer, sizeof(buffer));

	if (bytes_read > 0)
	{
		if (_output_state == OUTPUT_READING_HEADERS)
		{
			_output_buffer.append(buffer, bytes_read);
			size_t header_end = _output_buffer.find("\r\n\r\n");
			if (header_end != std::string::npos)
			{
				_output_state = OUTPUT_READING_BODY;
				std::stringstream ss;
				ss << "/tmp/webserv_cgi_output_" << time(NULL) << "_" << _pid;
				_output_file = ss.str();
				cgi_output_fd = open(_output_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
				if (cgi_output_fd == -1)
				{
					std::cerr << "[CGI] Failed to open output file: " << _output_file << std::endl;
					return false;
				}
				std::string headers_part = _output_buffer.substr(0, header_end);
				std::string body_part = _output_buffer.substr(header_end + 4);
				int written = write(cgi_output_fd, body_part.c_str(), body_part.size());
				if (written > 0)
					body_size += written;
				_output_buffer = headers_part;
			}
			if (_output_buffer.size() > 8192)
			{
				std::cerr << "[CGI] Headers too large or malformed. Aborting." << std::endl;
				_state = CGI_ERROR;
				return false;
			}
		}
		else if (_output_state == OUTPUT_READING_BODY)
		{
			int written = write(cgi_output_fd, buffer, bytes_read);
			if (written > 0)
				body_size += written;
		}
		return true;
	}
	else if (bytes_read == 0)
	{
		_state = CGI_COMPLETE;
		int status;
		if (waitpid(_pid, &status, WNOHANG) > 0)
		{
			if (WIFEXITED(status))
			{
				_exit_status = WEXITSTATUS(status);
			}
			_pid = -1;
		}
		close(cgi_output_fd);
		cgi_output_fd = -1;
		return false;
	}
	else
		return true;
}

bool CgiHandler::checkTimeout(int timeout_seconds)
{
	if (_state == CGI_RUNNING)
	{
		if (std::time(NULL) - _start_time > timeout_seconds)
		{
			killProcess();
			waitpid(_pid, NULL, 0);
			_pid = -1;
			_state = CGI_ERROR;
			return true;
		}
	}
	return false;
}

void CgiHandler::killProcess()
{
	if (_pid > 0)
	{
		kill(_pid, SIGKILL);
	}
}

pid_t CgiHandler::getPid() const { return _pid; }
int CgiHandler::getStdoutFd() const { return _stdout_pipe[0]; }
CgiState CgiHandler::getState() const { return _state; }
int CgiHandler::getMethod() const { return _method; }
std::string CgiHandler::getOutput() const { return _output_buffer; }
int CgiHandler::getExitStatus() const { return _exit_status; }

char **CgiHandler::_vecToCharArray(const std::vector<std::string> &vec) const
{
	char **arr = new char *[vec.size() + 1];
	for (size_t i = 0; i < vec.size(); ++i)
	{
		arr[i] = new char[vec[i].size() + 1];
		std::strcpy(arr[i], vec[i].c_str());
	}
	arr[vec.size()] = NULL;
	return arr;
}

void CgiHandler::_freeCharArray(char **arr) const
{
	if (!arr)
		return;
	for (int i = 0; arr[i] != NULL; ++i)
	{
		delete[] arr[i];
	}
	delete[] arr;
}