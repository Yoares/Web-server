#include "../../inc/config/ConfigValidator.hpp"
#include <sys/stat.h> // For stat()
#include <unistd.h>   // For access(), F_OK, R_OK, X_OK


void ConfigValidator::validate(const std::vector<Server>& servers) {

    if (servers.empty()){
        throw std::runtime_error("Validation Error: No servers configured.");
    }

    for (size_t i = 0; i < servers.size(); ++i){
        validate_server(servers[i]);
    }
}

void ConfigValidator::validate_server(const Server& srv) {
    // 1. Validate Port Range
    if (srv.listen_list.empty()) {
        throw std::runtime_error("Validation Error: Server block is missing a 'listen' directive.");
    }
    
    for (size_t i = 0; i < srv.listen_list.size(); ++i) {
        if (srv.listen_list[i].port <= 0 || srv.listen_list[i].port > 65535) {
            // Note: C++98 doesn't easily let us concatenate ints to strings in exceptions,
            // so a generic but clear message is perfect here.
            throw std::runtime_error("Validation Error: Invalid port number detected in a listen directive.");
        }
    }

    // 2. Validate Error Pages
    for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); 
         it != srv.error_pages.end(); ++it) {
        
        // Ensure the error page physically exists and is readable
        if (!is_file_accessible(it->second, R_OK)) {
            throw std::runtime_error("Validation Error: Error page not found or unreadable: " + it->second);
        }
    }

    // 3. Validate Locations
    for (size_t i = 0; i < srv.locations.size(); ++i) {
        validate_location(srv.locations[i]);
    }
}

void ConfigValidator::validate_location(const Location& loc) {
    // 1. Validate the Root Directory
    if (!loc.root.empty() && !is_directory(loc.root)) {
        throw std::runtime_error("Validation Error: Root directory does not exist or is not a directory: " + loc.root);
    }

    // 2. Validate Allowed Methods
    for (size_t i = 0; i < loc.allowed_methods.size(); ++i) {
        const std::string& method = loc.allowed_methods[i];
        if (method != "GET" && method != "POST" && method != "DELETE") {
            throw std::runtime_error("Validation Error: Unsupported HTTP method: " + method);
        }
    }

    // 3. Validate CGI Binaries
    for (std::map<std::string, std::string>::const_iterator it = loc.cgi_pass.begin(); 
         it != loc.cgi_pass.end(); ++it) {
        
        // Ensure the interpreter (like /usr/bin/php-cgi) exists and is EXECUTABLE
        if (!is_file_accessible(it->second, X_OK)) {
            throw std::runtime_error("Validation Error: CGI binary not found or not executable: " + it->second);
        }
    }

    // 4. Validate Upload Directory
    if (!loc.upload_dir.empty() && !is_directory(loc.upload_dir)) {
        throw std::runtime_error("Validation Error: Upload directory does not exist: " + loc.upload_dir);
    }
}

// --- POSIX System Call Wrappers ---

bool ConfigValidator::is_directory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return S_ISDIR(info.st_mode); // <-- Cleaner, standard POSIX macro
}

bool ConfigValidator::is_file_accessible(const std::string& path, int mode) {
    // access() checks real user's permissions for a file. 
    // mode can be F_OK (exists), R_OK (read), W_OK (write), X_OK (execute).
    return (access(path.c_str(), mode) == 0);
}