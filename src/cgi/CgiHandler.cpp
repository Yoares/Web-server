#include "../../inc/cgi/CgiHandler.hpp"

#include <unistd.h>     
#include <sys/wait.h>   
#include <sys/stat.h>   
#include <fcntl.h>     
#include <errno.h>
#include <cstdlib>     
#include <cstring>      
#include <sstream>
#include <fstream>
#include <stdexcept>


CgiHandler::CgiHandler(
    HttpRequest&        request,
    HttpResponse&       response,
    const Location&     location,
    const Server&       server,
    const std::string&  cgi_bin,
    const std::string&  script_path)
    : _request(request),
      _response(response),
      _location(location),
      _server(server),
      _cgi_bin(cgi_bin),
      _script_path(script_path)
{}

void CgiHandler::execute() {

    //  Verify script exists 
    struct stat st;
    if (stat(_script_path.c_str(), &st) == -1 || !S_ISREG(st.st_mode)) {
        _response.buildErrorResponse(404, _server.error_pages);
        return;
    }
    // Resolve to absolute path so SCRIPT_FILENAME is always correct
    char resolved[4096];
    if (realpath(_script_path.c_str(), resolved) != NULL)
        _script_path = resolved;

    std::vector<std::string> envVec = _buildEnv();
    char** envp = _toCharArray(envVec);

    std::vector<std::string> argVec;
    argVec.push_back(_cgi_bin);
    argVec.push_back(_script_path);
    char** argv = _toCharArray(argVec);

    int stdin_pipe[2]  = {-1, -1};
    int stdout_pipe[2] = {-1, -1};
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        _freeCharArray(envp,  envVec.size());
        _freeCharArray(argv,  argVec.size());
        _response.buildErrorResponse(500, _server.error_pages);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        _freeCharArray(envp, envVec.size());
        _freeCharArray(argv, argVec.size());
        _response.buildErrorResponse(500, _server.error_pages);
        return;
    }

    if (pid == 0) {
        // Redirect stdin ← read end of stdin_pipe
        if (dup2(stdin_pipe[0], STDIN_FILENO) == -1)  _exit(1);
        // Redirect stdout → write end of stdout_pipe
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) _exit(1);

        // Close all pipe ends we no longer need
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);

        // Change to script's directory (many CGI scripts expect this)
        std::string script_dir = _script_path.substr(0, _script_path.find_last_of('/'));
        if (!script_dir.empty())
            chdir(script_dir.c_str());

        execve(_cgi_bin.c_str(), argv, envp);
        // If execve returns, something went wrong
        _exit(1);
    }

    _freeCharArray(envp, envVec.size());
    _freeCharArray(argv, argVec.size());

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    const std::string& tempFile = _request.getTempFilename();
    if (!tempFile.empty() && _request.getContentLength() > 0) {
        std::ifstream bodyFile(tempFile.c_str(), std::ios::binary);
        if (bodyFile.is_open()) {
            char buf[4096];
            while (bodyFile.read(buf, sizeof(buf)) || bodyFile.gcount() > 0) {
                ssize_t written = write(stdin_pipe[1], buf, (size_t)bodyFile.gcount());
                (void)written; // errors here are non-fatal; CGI will get EOF
            }
        }
    }
    close(stdin_pipe[1]); // Signal EOF to child

    // Reading CGI output 
    std::string cgiOutput = _readAll(stdout_pipe[0]);
    close(stdout_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        _response.buildErrorResponse(502, _server.error_pages);
        return;
    }

    //Parsin' CGI output into response 
        _response.buildErrorResponse(502, _server.error_pages);
        return;
    }
    _parseCgiOutput(cgiOutput);
}

std::vector<std::string> CgiHandler::_buildEnv() const {
    std::vector<std::string> env;

    #define ENV(k, v) env.push_back(std::string(k) + "=" + (v))
    ENV("GATEWAY_INTERFACE", "CGI/1.1");
    ENV("SERVER_PROTOCOL",   "HTTP/1.1");
    ENV("SERVER_SOFTWARE",   "Webserv/1.0");

    std::string methodStr;
    switch (_request.getMethod()) {
        case GET:    methodStr = "GET";    break;
        case POST:   methodStr = "POST";   break;
        case DELETE: methodStr = "DELETE"; break;
        default:     methodStr = "GET";    break;
    }
    ENV("REQUEST_METHOD",  methodStr);
    ENV("SCRIPT_FILENAME", _script_path);
    ENV("SCRIPT_NAME",     _request.getPath());
    ENV("PATH_INFO",       _request.getPath());
    ENV("QUERY_STRING",    _request.getQueryString());
    // We don't have direct access to the socket here, leaving reasonable defaults
    ENV("SERVER_NAME", "localhost");
    ENV("SERVER_PORT", "8080");

    std::map<std::string, std::string> headers = _request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        std::string key = it->first;
        for (size_t i = 0; i < key.size(); ++i) {
            key[i] = (key[i] == '-') ? '_' : std::toupper(key[i]);
        }
        if (key == "CONTENT_TYPE") {
            ENV("CONTENT_TYPE", it->second);
        } else if (key == "CONTENT_LENGTH") {
            ENV("CONTENT_LENGTH", it->second);
        } else {
            env.push_back("HTTP_" + key + "=" + it->second);
        }
    }

    if (headers.find("content-length") == headers.end() &&
        headers.find("Content-Length") == headers.end() &&
        _request.getContentLength() > 0)
    {
        std::ostringstream ss;
        ss << _request.getContentLength();
        ENV("CONTENT_LENGTH", ss.str());
    }

    if (!_location.upload_dir.empty())
        ENV("UPLOAD_DIR", _location.upload_dir);

    if (!_location.root.empty())
        ENV("DOCUMENT_ROOT", _location.root);

    #undef ENV
    return env;
}

void CgiHandler::_parseCgiOutput(const std::string& raw) const {

    size_t sep = raw.find("\r\n\r\n");
    size_t headerLen = 4;
    if (sep == std::string::npos) {
        sep = raw.find("\n\n");
        headerLen = 2;
    }
    if (sep == std::string::npos) {
        _response.setStatusCode(200);
        _response.setBody(raw);
        return;
    }

    std::string headerSection = raw.substr(0, sep);
    std::string body          = raw.substr(sep + headerLen);

    //Parsing CGI headers 
    int statusCode = 200;
    size_t pos = 0;
    while (pos < headerSection.size()) {
        size_t eol = headerSection.find('\n', pos);
        if (eol == std::string::npos) eol = headerSection.size();

        std::string line = headerSection.substr(pos, eol - pos);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        pos = eol + 1;
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string hname = line.substr(0, colon);
        std::string hval  = line.substr(colon + 1);

        size_t vs = hval.find_first_not_of(" \t");
        if (vs != std::string::npos) hval = hval.substr(vs);

        if (hname == "Status") {
            statusCode = std::atoi(hval.c_str());
            _response.setStatusCode(statusCode);
        } else {
            _response.setHeader(hname, hval);
        }
    }

    _response.setStatusCode(statusCode);
    _response.setBody(body);
}

// Read everything from fd until EOF
std::string CgiHandler::_readAll(int fd) const {
    std::string result;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        result.append(buf, (size_t)n);
    }
    return result;
}
// Converting vector<string> → char** (NULL-terminated)
char** CgiHandler::_toCharArray(const std::vector<std::string>& vec) const {
    char** arr = new char*[vec.size() + 1];
    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = new char[vec[i].size() + 1];
        std::memcpy(arr[i], vec[i].c_str(), vec[i].size() + 1);
    }
    arr[vec.size()] = NULL;
    return arr;
}

void CgiHandler::_freeCharArray(char** arr, size_t count) const {
    for (size_t i = 0; i < count; ++i)
        delete[] arr[i];
    delete[] arr;
}