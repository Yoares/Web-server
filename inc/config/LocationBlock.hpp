#ifndef LOCATIONBLOCK_HPP
#define LOCATIONBLOCK_HPP
#include <string>
#include <vector>
#include <map>
#include <iostream>

struct Location {
    std::string path; //Routing: path and root cover translating URIs to physical directories.
    std::string root;
    std::string index;
    
    std::vector<std::string> allowed_methods; //Security: allowed_methods restricts bad requests (e.g., stopping a POST on a read-only directory).
    
    bool autoindex; //Files: index and autoindex handle directory requests perfectly.
    bool upload_enable; //Uploads: upload_enable and upload_dir handle the file upload requirement cleanly.
    std::string upload_dir;
    
    int redirect_code;         
    std::string redirect_url;
    
    std::map<std::string, std::string> cgi_pass; //CGI: cgi_pass mapping (.php -> /usr/bin/php-cgi) 


};

#endif