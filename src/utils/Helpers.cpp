#include "../../inc/utils/Helpers.hpp"

std::string decodeURI(const std::string &encoded_uri)
{
	std::string decoded;
	decoded.reserve(encoded_uri.length());

	for (size_t i = 0; i < encoded_uri.length(); ++i)
	{
		if (encoded_uri[i] == '%')
		{
			if (i + 2 < encoded_uri.length())
			{
				std::string hexStr = encoded_uri.substr(i + 1, 2);
				char decodedChar = static_cast<char>(std::strtol(hexStr.c_str(), NULL, 16));
				decoded += decodedChar;
				i += 2;
			}
			else
				decoded += encoded_uri[i];
		}
		else if (encoded_uri[i] == '+')
			decoded += ' ';
		else
			decoded += encoded_uri[i];
	}
	return decoded;
}

std::string encodeURI(const std::string &raw_uri)
{
	std::ostringstream encoded;
	encoded.fill('0');
	encoded << std::hex << std::uppercase;

	for (size_t i = 0; i < raw_uri.length(); ++i)
	{
		unsigned char c = raw_uri[i];
		if ((c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '-' || c == '_' || c == '.' || c == '~')
			encoded << c;
		else
			encoded << '%' << std::setw(2) << static_cast<int>(c);
	}
	return encoded.str();
}
