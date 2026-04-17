#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>

class Tokenizer {
public:
    // Takes a file path, returns a clean array of tokens
    static std::vector<std::string> tokenize(const std::string& filename);
};

#endif