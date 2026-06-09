#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <vector>
#include <sys/types.h>
#include <ctime>
#include "../http/HttpRequest.hpp"
// State Machine for asynchronous epoll monitoring
enum CgiState {
    CGI_INIT,       // Initialized, but fork() hasn't been called
    CGI_RUNNING,    // Child is executing, waiting for stdout
    CGI_COMPLETE,   // Process finished, all output read
    CGI_ERROR       // Failed to execute or timeout
};

class CgiHandler {
public:
    // Constructor requires only the data needed to build the execution context
    CgiHandler(const std::string& script_path, 
               const std::string& cgi_bin, 
               int method, 
               const std::string& tmp_post_file);
    
	CgiHandler(){
		_pid = -1;
		_method = 0;
		_state = CGI_INIT;
		_script_path = "";
		_cgi_bin = "";
		_tmp_post_file = "";
		_exit_status = 0;
		_stdout_pipe[0] = -1;
		_stdout_pipe[1] = -1;
	}
	CgiHandler& operator=(const CgiHandler& other){
		this->_pid = -1;
		this->_method = other._method;
		this->_state = other._state;
		this->_script_path = other._script_path;
		this->_cgi_bin = other._cgi_bin;
		this->_tmp_post_file = other._tmp_post_file;
		this->_exit_status = other._exit_status;
		this->_stdout_pipe[0] = -1;
		this->_stdout_pipe[1] = -1;
		return *this;}
    ~CgiHandler();

    // Core Actions
    bool    execute(const std::vector<std::string>& envp_vec);
    bool    readOutputNonBlocking(); 
    void    killProcess();
    bool    checkTimeout(int timeout_seconds = 5); // Prevent infinite loops!

    // Getters for epoll to monitor
    pid_t           getPid() const;
    int             getStdoutFd() const;
    CgiState        getState() const;
    int             getMethod() const;
    
    // Result extraction
    std::string     getOutput() const;
    int             getExitStatus() const;

private:
    // Core Variables requested
    pid_t           _pid;           // The Child Process ID
    int             _method;        // GET (1), POST (2), etc.
    CgiState        _state;         // Current state in the state machine
    
    // File Descriptors
    int             _stdout_pipe[2]; // Parent reads from [0], Child writes to [1]
    
    // Execution Context
    std::string     _script_path;
    std::string     _cgi_bin;
    std::string     _tmp_post_file;  // The /tmp/webserv_XX file for POST stdin
    
    // Data Accumulation
    std::string     _output_buffer;  // Assembled output from the CGI
    int             _exit_status;    // Return code of the script
    time_t          _start_time;     // For timeout tracking

    // Helper memory managers
    char** _vecToCharArray(const std::vector<std::string>& vec) const;
    void            _freeCharArray(char** arr) const;
};

#endif