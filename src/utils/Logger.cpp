#include "../../inc/utils/Logger.hpp"

Logger_manager::Logger_manager() : _session_timeout(3600) {
    std::srand(std::time(NULL));
}

Logger_manager::~Logger_manager() {}

