#include "../../inc/core/Post.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <sys/time.h>

static std::string generateUniqueFilename(const std::string &baseName)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    std::ostringstream oss;

    oss << tv.tv_sec << "_" << tv.tv_usec << "_" << baseName; 
    
    return oss.str();
}

void PostHandler::buildSuccessResponse(const std::vector<std::string> &finalNames, bool isRaw)
{
    _response.setStatusCode(201);

    std::string base = _request.getPath();
    if (!base.empty() && base[base.size() - 1] == '/')
        base.erase(base.size() - 1);

    if (isRaw && !finalNames.empty())
    {
        _response.setHeader("Location", base + "/" + finalNames[0]);
    }
    else
    {
        _response.setHeader("Location", base);
    }

    std::ostringstream json;
    json << "{\n  \"status\": 201,\n  \"uploaded\": [";
    for (size_t i = 0; i < finalNames.size(); ++i)
    {
        json << "\n    \"" << base << "/" << finalNames[i] << "\"";
        if (i + 1 < finalNames.size())
            json << ",";
    }
    json << "\n  ]\n}";

    _response.setHeader("Content-Type", "application/json");
    _response.setBody(json.str());
}

static std::string getParentDirectory(
    const std::string &path)
{
    std::string::size_type pos =
        path.find_last_of('/');

    if (pos == std::string::npos)
        return "";

    return path.substr(0, pos);
}

PostHandler::PostHandler(HttpRequest &request, HttpResponse &response, const Server &server, const Location &location)
    : _request(request),
      _response(response),
      _server(server),
      _location(location)
{
}

bool PostHandler::validateBodySize(const std::string &temp_file)
{
    struct stat st;

    if (stat(temp_file.c_str(), &st) == -1)
    {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }

    if (static_cast<size_t>(st.st_size) > _location.client_max_body_size)
    {
        _response.buildErrorResponse(413, _server.error_pages);
        return false;
    }

    return true;
}

bool PostHandler::validateUploadDirectory(const std::string &path)
{
    std::string dir = getParentDirectory(path);

    if (dir.empty())
    {
        _response.buildErrorResponse(400, _server.error_pages);
        return false;
    }

    struct stat st;
    if (stat(dir.c_str(), &st) == -1)
    {
        _response.buildErrorResponse(404, _server.error_pages);
        return false;
    }
    if (!S_ISDIR(st.st_mode))
    {
        _response.buildErrorResponse(403, _server.error_pages);
        return false;
    }
    if (access(dir.c_str(), W_OK) == -1)
    {
        _response.buildErrorResponse(403, _server.error_pages);
        return false;
    }
    return true;
}

bool PostHandler::copyToDestination(const std::string &temp_file, const std::string &path)
{
    struct stat st;

    if (stat(path.c_str(), &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            _response.buildErrorResponse(403, _server.error_pages);
            return false;
        }
    }
    int src_fd = open(temp_file.c_str(), O_RDONLY);
    if (src_fd == -1)
    {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }

    int dst_fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dst_fd == -1)
    {
        close(src_fd);
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }

    char buffer[4096];
    ssize_t bytesRead;
    bool success = true;

    while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0)
    {
        ssize_t totalWritten = 0;
        while (totalWritten < bytesRead)
        {
            ssize_t written = write(dst_fd, buffer + totalWritten, bytesRead - totalWritten);
            if (written == -1)
            {
                success = false;
                break;
            }
            totalWritten += written;
        }
        if (!success)
            break;
    }

    if (bytesRead == -1)
    {
        success = false;
    }

    close(src_fd);
    close(dst_fd);

    if (!success)
    {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }

    return true;
}

bool PostHandler::isMultipart() const
{
    std::map<std::string, std::string> headers = _request.getHeaders();
    std::map<std::string, std::string>::iterator it;

    it = headers.find("Content-Type");

    if (it == headers.end())
    {
        it = headers.find("content-type");
    }

    if (it == headers.end())
    {
        std::cout << "isMultipart: No Content-Type header found at all." << std::endl;
        return false;
    }

    return (it->second.find("multipart/form-data") != std::string::npos);
}

std::string PostHandler::extractBoundary(const std::string &contentType) const
{
    std::string key = "boundary=";

    std::string::size_type pos = contentType.find(key);

    if (pos == std::string::npos)
        return "";

    return contentType.substr(pos + key.length());
}

bool PostHandler::processMultipart(const std::string &temp_file, const std::string &boundary, const std::string &upload_dir, std::vector<std::string> &out_filenames)
{

    std::ifstream infile(temp_file.c_str(), std::ios::binary);
    if (!infile.is_open())
    {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }
    std::string end_boundary = "\r\n--" + boundary;

    std::vector<char> buffer;
    char chunk[4096];

    bool headers_parsed = false;
    bool is_file = false;
    std::ofstream outfile;
    std::string filename = "";

    while (infile.read(chunk, sizeof(chunk)))
    {
        
    }
}

void PostHandler::execute(std::string path)
{

    if (path.empty())
    {
        _response.buildErrorResponse(400, _server.error_pages);
        return;
    }

    struct stat st;
    if (stat("www/html/uploads", &st) == -1)
    {
        if (mkdir("www/html/uploads", 0777) == -1)
        {
            _response.buildErrorResponse(500, _server.error_pages);
            return;
        }
    }

    if (!validateUploadDirectory(path))
        return;

    if (_request.getContentLength() == 0)
    {
        _response.setStatusCode(200);
        _response.setBody("");
        return;
    }

    std::string temp_file = _request.getTempFilename();

    if (!validateBodySize(temp_file))
        return;

    std::vector<std::string> uploaded_files;
    if (isMultipart())
    {
        std::map<std::string, std::string> headers = _request.getHeaders();

        std::string ct_val = "";
        if (headers.find("Content-Type") != headers.end())
            ct_val = headers["Content-Type"];
        else if (headers.find("content-type") != headers.end())
            ct_val = headers["content-type"];

        std::string boundary = extractBoundary(ct_val);
        if (boundary.empty())
        {
            _response.buildErrorResponse(400, _server.error_pages);
            return;
        }

        std::vector<std::string> parsed_filenames;

        if (!processMultipart(temp_file, boundary, path, parsed_filenames))
        {
            return;
        }

        uploaded_files = parsed_filenames;
        buildSuccessResponse(uploaded_files, false);
    }
    else
    {

        std::string filename;
        if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        {
            filename = generateUniqueFilename("uploaded_raw_file.bin"); 
            path += "/" + filename;
        }
        else
        {
            size_t pos = path.find_last_of('/');
            filename = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        }
        if (!copyToDestination(temp_file, path))
        {
            return;
        }
        uploaded_files.push_back(filename);
        buildSuccessResponse(uploaded_files, true);
    }
}
