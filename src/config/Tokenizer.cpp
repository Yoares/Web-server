#include "../../inc/config/Tokenizer.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

std::vector<std::string> Tokenizer::tokenize(const std::string& filename){
    std::vector<std::string> tokens;

    std::ifstream file(filename.c_str());

    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open config file: " + filename);
    }

    std::string content;
    std::ostringstream ss;
    ss << file.rdbuf();
    content = ss.str();

    size_t i = 0;

    while (i < content.length())
    {
        if (std::isspace(content[i])){
            i++;
            continue;
        }

        if (content[i] == '#')
        {
            while(i < content.length() && content[i] != '\n')
                i++;
            continue;
        }

        if (content[i] == '{' || content[i] == '}' || content[i] == ';')
        {
            tokens.push_back(std::string(1, content[i]));
            i++;
            continue;
        }
        size_t start = i;
        while(i < content.length() && !std::isspace(content[i]) && content[i] != '{' && content[i] != '}' && content[i] != ';' && content[i] != '#')
        {
            i++;
        } 

        // Push the extracted word into our vector
        tokens.push_back(content.substr(start, i - start));
    }
    return tokens;
}