#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

std::string decodeURI(const std::string &encoded_uri);
std::string encodeURI(const std::string &raw_uri);

template <typename T>
std::string to_string(T value) {
    std::ostringstream os;
    os << value;
    return os.str();
}
#endif