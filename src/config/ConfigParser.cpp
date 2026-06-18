#include "../../inc/config/ConfigParser.hpp"
#include <cstdlib>
#include <stdexcept>

ConfigParser::ConfigParser(const std::vector<std::string> &tokens) : _tokens(tokens), _pos(0) {}

std::string ConfigParser::consume()
{
    if (_pos >= _tokens.size())
    {
        throw std::runtime_error("Config Error: Unexpected end of file.");
    }
    return _tokens[_pos++];
}

void ConfigParser::expect(const std::string &expected)
{
    if (consume() != expected)
    {
        throw std::runtime_error("Config Syntax Error: Expected '" + expected + "' near token position in file.");
    }
}

// The Main Loop
std::vector<Server> ConfigParser::parse()
{
    std::vector<Server> servers;

    while (_pos < _tokens.size())
    {
        std::string token = consume();
        if (token == "server")
        {
            servers.push_back(parse_server());
        }
        else
        {
            throw std::runtime_error("Config Syntax Error: Unknown directive '" + token + "' outside of server block.");
        }
    }

    if (servers.empty())
    {
        throw std::runtime_error("Config Error: No server blocks found.");
    }

    return servers;
}

// Parses everything inside server { ... }
Server ConfigParser::parse_server()
{
    Server srv;
    expect("{");

    while (_pos < _tokens.size() && _tokens[_pos] != "}")
    {
        std::string directive = consume();

        if (directive == "listen")
        {
            std::string listen_val = consume();
            std::string ip = "0.0.0.0";
            int port = 80;
            size_t colon_pos = listen_val.find(':');
            if (colon_pos != std::string::npos)
            {
                ip = listen_val.substr(0, colon_pos);
                port = std::atoi(listen_val.substr(colon_pos + 1).c_str());
            }
            else
            {
                port = std::atoi(listen_val.c_str());
            }

            srv.listen_list.push_back(ListenParams(ip, port));
            expect(";");
        }
        else if (directive == "server_name")
        {
            while (_pos < _tokens.size() && _tokens[_pos] != ";")
            {
                srv.server_names.push_back(consume());
            }
            expect(";");
        }
        else if (directive == "root")
        {
            srv.root = (consume().c_str());
            expect(";");
        }
        else if (directive == "index")
        {
            srv.index = (consume().c_str());
            expect(";");
        }
        else if (directive == "client_max_body_size")
        {
            srv.client_max_body_size = (size_t)std::atoi(consume().c_str());
            expect(";");
        }
        else if (directive == "error_page")
        {
            int code = std::atoi(consume().c_str());
            std::string page = consume();
            srv.error_pages[code] = page;
            expect(";");
        }
        else if (directive == "location")
        {
            srv.locations.push_back(parse_location(srv.client_max_body_size, srv.root, srv.index));
        }
        else
        {
            throw std::runtime_error("Config Syntax Error: Unknown server directive '" + directive + "'");
        }
    }
    expect("}");

    return srv;
}

// Parses everything inside location /path { ... }
Location ConfigParser::parse_location(size_t inherited_max_body_size, std::string &inher_root, std::string &inher_index)
{
    Location loc;

    loc.client_max_body_size = inherited_max_body_size;
    loc.root = inher_root;
    loc.index = inher_index;

    loc.path = consume();
    expect("{");

    while (_pos < _tokens.size() && _tokens[_pos] != "}")
    {
        std::string directive = consume();

        if (directive == "root")
        {
            loc.root = consume();
            expect(";");
        }
        else if (directive == "index")
        {
            loc.index = consume();
            expect(";");
        }
        else if (directive == "autoindex")
        {
            std::string val = consume();
            loc.autoindex = (val == "on");
            expect(";");
        }
        else if (directive == "allow_methods")
        {
            while (_pos < _tokens.size() && _tokens[_pos] != ";")
            {
                loc.allowed_methods.push_back(consume());
            }
            expect(";");
        }
        else if (directive == "cgi_pass")
        {
            std::string ext = consume();
            std::string bin = consume();
            loc.cgi_pass[ext] = bin;
            expect(";");
        }
        else if (directive == "upload_dir" || directive == "upload_store")
        {
            loc.upload_dir = consume();
            expect(";");
        }
        else if (directive == "return" || directive == "redirect")
        {
            loc.redirect_code = std::atoi(consume().c_str());
            loc.redirect_url = consume();
            expect(";");
        }
        else if (directive == "client_max_body_size")
        {
            loc.client_max_body_size = (size_t)std::atoi(consume().c_str());
            expect(";");
        }
        else
        {
            throw std::runtime_error("Config Syntax Error: Unknown location directive '" + directive + "'");
        }
    }
    expect("}");

    return loc;
}