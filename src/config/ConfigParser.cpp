#include "../../inc/config/ConfigParser.hpp"
#include <cstdlib> // For atoi()

ConfigParser::ConfigParser(const std::vector<std::string>& tokens) : _tokens(tokens) {};

std::string ConfigParser::consume(){
    if (_pos >= _tokens.size())
    {
        throw std::runtime_error("Config Error: Unexpected end of file.");
    }
    return _tokens[_pos++];
}

void ConfigParser::expect(const std::string& expected) {
    if (consume() != expected){
        throw std::runtime_error("Config Syntax Error: Expected '" + expected + "' near token position " + "in file.");
    } 
}

// The Main Loop
std::vector<Server> ConfigParser::parse() {
    std::vector<Server> servers;

    while(_pos < _tokens.size()){
        std::string token = consume();
        if (token == "server"){
            servers.push_back(parse_server());
        } else {
            throw std::runtime_error("Config Syntax Error: Unknown directive '" + token + "' outside of server block.");
        }
    }
    if (servers.empty()) {
        throw std::runtime_error("Config Error: No server blocks found.");
    }

    return servers;
}

// Parses everything inside server { ... }
Server ConfigParser::parse_server() {
    Server srv;
    expect("{");

    while (_pos < _tokens.size() && _tokens[_pos] != "}") {
        std::string directive = consume();

        if (directive ==  "listen"){
            srv.port = std::atoi(consume().c_str());
            expect(";");
        }
        else if (directive == "server_name"){
            srv.server_names.push_back(consume());
            expect(";");
        }
        else if (directive == "client_max_body_size"){
            srv.client_max_body_size = (size_t)std::atoi(consume().c_str());
            expect(";");
        }
        else if (directive == "error_page"){
            int code = std::atoi(consume().c_str());
            std::string page = consume();
            srv.error_pages[code] = page;
            expect(";");
        }
        else if (directive ==  "location"){
            srv.locations.push_back(parse_location());
        }
        else {
            throw std::runtime_error("Config Syntax Error: Unknown server directive '" + directive + "'");
        }
    }
    expect("}");

    return srv;
}

// Parses everything inside location /path { ... }
Location ConfigParser::parse_location() {
    Location loc;

    loc.path = consume();
    expect("{");

    while (_pos < _tokens.size() && _tokens[_pos] != "}") {
        std::string directive = consume();
        
        if (directive == "root"){
            loc.root = consume();
            expect(";");
        }
        else if (directive == "index"){
            loc.index = consume();
            expect(";");
        }
        else if (directive == "autoindex"){
            std::string val = consume();
            loc.autoindex = (val == "on");
            expect(";");
        }
        else if (directive == "allow_methods"){
            // allow_methods can have multiple values before the semicolon
            while (_pos < _tokens.size() && _tokens[_pos] != ";") {
                loc.allowed_methods.push_back(consume());
            }
            expect(";");
        }
        else if (directive == "cgi_pass"){
            std::string ext = consume();
            std::string bin = consume();
            loc.cgi_pass[ext] = bin;
            expect(";");
        }
        else if (directive == "upload_enable") {
            std::string val = consume();
            loc.upload_enable = (val == "on");
            expect(";");
        }
        else if (directive == "upload_dir" || directive == "upload_store") {
            loc.upload_dir = consume();
            expect(";");
        }
        else if (directive == "return" || directive == "redirect") {
            // NGINX uses 'return' for redirects, e.g., "return 301 /new_path;"
            loc.redirect_code = std::atoi(consume().c_str()); // Grab the 301
            loc.redirect_url = consume();                     // Grab the /new_path
            expect(";");
        }
        else {
            throw std::runtime_error("Config Syntax Error: Unknown location directive '" + directive + "'");
        }
    }
    expect("}");
    
    return loc;
}
