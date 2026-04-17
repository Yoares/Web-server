#include "../../inc/config/ServerBlock.hpp"

// --- C++98 Default Constructor ---
    // Sets safe fallbacks to prevent undefined behavior
    Server::Server() : 
        ip("0.0.0.0"), 
        port(80),                      // Standard HTTP port fallback
        client_max_body_size(1048576), // 1MB default limit
        socket_fd(-1)                  // -1 indicates the socket isn't open yet
    {}