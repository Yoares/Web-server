#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <string>

class MimeTypes
{
    public:
        // Returns the MIME type based on file extension
        static std::string getMimeType(const std::string& path);
};

#endif