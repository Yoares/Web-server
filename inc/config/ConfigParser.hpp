#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include "ServerBlock.hpp" 
#include "LocationBlock.hpp"

class ConfigParser {
private:
    std::vector<std::string> _tokens;
    size_t _pos; // Our current position in the token array

    // --- Core Parsing Engine ---
    std::string consume();
    void expect(const std::string& expected);

    // --- Block Parsers ---
    Server parse_server();
    Location parse_location();

public:
    // Constructor takes the tokens from the Tokenizer
    ConfigParser(const std::vector<std::string>& tokens);
    
    // Returns the fully built vector of servers
    std::vector<Server> parse(); 
};

#endif