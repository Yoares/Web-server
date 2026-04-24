#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <sstream>

template <typename T>
std::string to_string(T value) {
    std::ostringstream os;
    os << value;
    return os.str();
}
#endif