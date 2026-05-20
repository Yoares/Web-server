#include "../../inc/core/Post.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <fcntl.h>
#include <sstream> 

void PostHandler::buildSuccessResponse(const std::vector<std::string>& finalNames, bool isRaw) {
    _response.setStatusCode(201);

    // Use the exact URI the client requested to build the Location header
    std::string base = _request.getPath();
    if (!base.empty() && base[base.size() - 1] == '/')
        base.erase(base.size() - 1);

    // Set the Location header
    if (isRaw && !finalNames.empty()) {
        _response.setHeader("Location", base + "/" + finalNames[0]);
    } else {
        _response.setHeader("Location", base);
    }

    // Build the JSON string
    std::ostringstream json;
    json << "{\n  \"status\": 201,\n  \"uploaded\": [";
    for (size_t i = 0; i < finalNames.size(); ++i) {
        json << "\n    \"" << base << "/" << finalNames[i] << "\"";
        if (i + 1 < finalNames.size()) json << ",";
    }
    json << "\n  ]\n}";

    // Attach the JSON to your HttpResponse
    _response.setHeader("Content-Type", "application/json");
    _response.setBody(json.str());
}

std::string PostHandler::resolvePhysicalPath(const std::string& request_uri, const Location& loc) {
    std::string _path;
    std::string root ;

    if (loc.root.empty())
        root = "/var/www/html";
    else
        root = loc.root;

    if (request_uri.find("..") != std::string::npos)
        return "";

    // Prevent double slashes when joining
    if (root[root.length() - 1] == '/' && request_uri[0] == '/') {
        _path = root + request_uri.substr(1);
    } else if (root[root.length() - 1] != '/' && request_uri[0] != '/') {
        _path = root + "/" + request_uri;
    } else {
        _path = root + request_uri;
    }

    return _path;
}

static std::string getParentDirectory(
    const std::string& path)
{
    std::string::size_type pos =
        path.find_last_of('/');

    if (pos == std::string::npos)
        return "";

    return path.substr(0, pos);
}

PostHandler::PostHandler(HttpRequest& request, HttpResponse& response, const Server& server, const Location& location)
    : _request(request),
      _response(response),
      _server(server),
      _location(location)
{

}

bool PostHandler::validateBodySize(const std::string& temp_file) {
    struct stat st;
    if (stat(temp_file.c_str(), &st) == -1) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;       
    }

    if (static_cast<size_t>(st.st_size) > _server.client_max_body_size) {
        _response.buildErrorResponse(413, _server.error_pages); // 413 Payload Too Large
        return false; 
    }
    return true;
}

bool PostHandler::validateUploadDirectory(const std::string& path){
    std::string dir = getParentDirectory(path);


if (dir.empty()) {
        _response.buildErrorResponse(400, _server.error_pages);
        return false;
    }

    struct stat st;
    if (stat(dir.c_str(), &st) == -1) {
        _response.buildErrorResponse(404, _server.error_pages);
        return false; 
    }
    if (!S_ISDIR(st.st_mode)) {
        _response.buildErrorResponse(403, _server.error_pages);
        return false;        
    }
    if (access(dir.c_str(), W_OK) == -1) {
        _response.buildErrorResponse(403, _server.error_pages);
        return false;  
    }
    return true;
}

bool PostHandler::copyToDestination(const std::string& temp_file, const std::string& path) {
    struct stat st;

    if (stat(path.c_str(), &st) == 0) { 
        if (S_ISDIR(st.st_mode)) {
            _response.buildErrorResponse(403, _server.error_pages);
            return false;
        }
    }
    int src_fd = open(temp_file.c_str(), O_RDONLY);
    if (src_fd == -1) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }

    int dst_fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dst_fd == -1) {
        close(src_fd);
        _response.buildErrorResponse(500, _server.error_pages);
        return false;       
    }

    char buffer[4096];
    ssize_t bytesRead;
    bool success = true;

    while ((bytesRead = read(src_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t totalWritten = 0;
        while (totalWritten < bytesRead) {
            ssize_t written = write(dst_fd, buffer + totalWritten, bytesRead - totalWritten);
            if (written == -1) {
                success = false;
                break;
            }
            totalWritten += written;
        }
        if (!success) break;
    }

    if (bytesRead == -1) {
        success = false;
    }

    close(src_fd);
    close(dst_fd);

    if (!success) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;   
    }

    return true;
}

bool PostHandler::isMultipart() const {
    std::map<std::string, std::string> headers = _request.getHeaders();
    std::map<std::string, std::string>::iterator it;

    // 1. Try finding it with exact capitalization
    it = headers.find("Content-Type");
    
    // 2. If not found, try finding it in all lowercase (very common in webserv!)
    if (it == headers.end()) {
        it = headers.find("content-type");
    }

    // 3. If STILL not found, it's definitely not multipart
    if (it == headers.end()) {
        std::cout << "isMultipart: No Content-Type header found at all." << std::endl;
        return false;
    }
    
    // 4. Check if the value contains our keyword
    return (it->second.find("multipart/form-data") != std::string::npos);
}

std::string PostHandler::extractBoundary(const std::string& contentType) const{
    std::string key = "boundary=";

    std::string::size_type pos = contentType.find(key);

    if (pos == std::string::npos)
        return "";

    return contentType.substr(pos + key.length());
}
// Step 1: Open temporary uploaded request body file
// Step 2: Read it chunk by chunk (streaming)
// Step 3: Parse multipart headers → extract filename
// Step 4: Stream file content to disk until boundary

bool PostHandler::processMultipart(const std::string& temp_file, const std::string& boundary,const std::string& upload_dir, std::string& out_filename){
    
    std::ifstream infile(temp_file.c_str(), std::ios::binary);
    if (!infile.is_open()) {
        _response.buildErrorResponse(500, _server.error_pages);
        return false;
    }
    std::string end_boundary = "\r\n--" + boundary;

    std::vector<char> buffer;
    char chunk[4096];
    bool headers_parsed = false;
    std::ofstream outfile;
    std::string filename = "uploaded_file.bin";

    while(infile.read(chunk, sizeof(chunk)) || infile.gcount() > 0){
        buffer.insert(buffer.end(), chunk, chunk + infile.gcount());
    
        // 2. State 1: Find the Headers and the Filename
        if (!headers_parsed){
            std::string header_end_str = "\r\n\r\n";
            std::vector<char>::iterator header_end = std::search(buffer.begin(), buffer.end(), header_end_str.begin(), header_end_str.end());
            if (header_end != buffer.end()){
                std::string header_text(buffer.begin(), header_end);
                size_t f_pos = header_text.find("filename=\"");
                if (f_pos != std::string::npos){
                    f_pos += 10;
                    size_t f_end = header_text.find("\"", f_pos);
                    if (f_end != std::string::npos){
                        filename = header_text.substr(f_pos, f_end - f_pos);
                    }
                }
            out_filename = filename;
            }
            std::string final_path = upload_dir;
            if (final_path[final_path.length() - 1] != '/') 
                final_path += "/";
            final_path += filename;
            outfile.open(final_path.c_str(), std::ios::binary | std::ios::trunc);
            if (!outfile.is_open()) {
                _response.buildErrorResponse(500, _server.error_pages);
                return false;
            }
            buffer.erase(buffer.begin(), header_end + 4);
            headers_parsed = true;
        }
        // 3. State 2: Write the File Content
        // 3. State 2: Write the File Content
    if (headers_parsed) {
        std::vector<char>::iterator boundary_pos = std::search(buffer.begin(), buffer.end(), end_boundary.begin(), end_boundary.end());
        
        if (boundary_pos != buffer.end()) {
            if(boundary_pos > buffer.begin()){
                outfile.write(&buffer[0], boundary_pos - buffer.begin());
            }
            break;
        } else { // <--- THIS MUST BE ATTACHED TO THE BOUNDARY CHECK
            if (buffer.size() > end_boundary.length()){
                size_t write_size = buffer.size() - end_boundary.length();
                outfile.write(&buffer[0], write_size);
                buffer.erase(buffer.begin(), buffer.begin() + write_size);
            }
        }
    }
    }
    if (outfile.is_open()){
        outfile.close();
    }
    infile.close();
    return true;
}

void PostHandler::execute() {
    std::string path = resolvePhysicalPath(_request.getPath(), _location);

    if (path.empty()) {
        _response.buildErrorResponse(400, _server.error_pages);
        return;
    }

    if (!validateUploadDirectory(path)) return;

    std::string temp_file = _request.getTempFilename();

    if (!validateBodySize(temp_file)) return;

    // --- NEW LOGIC STARTS HERE ---
    std::vector<std::string> uploaded_files;

    if (isMultipart()) {
        std::map<std::string, std::string> headers = _request.getHeaders();
        
        std::string ct_val = "";
        if (headers.find("Content-Type") != headers.end()) ct_val = headers["Content-Type"];
        else if (headers.find("content-type") != headers.end()) ct_val = headers["content-type"];

        std::string boundary = extractBoundary(ct_val);
        if (boundary.empty()) {
            _response.buildErrorResponse(400, _server.error_pages);
            return;
        }

        std::string parsed_filename;
        if (!processMultipart(temp_file, boundary, path, parsed_filename)) {
            return; // Error is handled inside
        }
        
        // Add the file to our success list
        uploaded_files.push_back(parsed_filename);
        buildSuccessResponse(uploaded_files, false);
    } 
    else {
        std::string filename;
        struct stat st;
        if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            filename = "uploaded_raw_file.bin";
            path += "/" + filename; 
        } else {
            // Extract the filename from the end of the path
            size_t pos = path.find_last_of('/');
            filename = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        }
        
        if (!copyToDestination(temp_file, path)) {
            return;
        }
        
        // Add the file to our success list
        uploaded_files.push_back(filename);
        buildSuccessResponse(uploaded_files, true);
    }
}
