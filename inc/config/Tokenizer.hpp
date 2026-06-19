#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>

class Tokenizer
{
	public:
		static std::vector<std::string> tokenize(const std::string &filename);
};

#endif