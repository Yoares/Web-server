#include "../../inc/utils/Logger.hpp"

Logger_manager::Logger_manager() : _session_timeout(3600)
{
    std::srand(std::time(NULL));
}

Logger_manager::~Logger_manager() {}

std::string Logger_manager::generateSessionID()
{
    const char alphanum[] = "012345679"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "abcdefghijklmnopqrstuvwxyz";
    int len = 32;
    std::string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i)
    {
        s += alphanum[std::rand() % (sizeof(alphanum) - 1)];
    }
    return s;
}

std::string Logger_manager::createSession()
{
    std::string new_id = generateSessionID();

    while (_sessions.find(new_id) != _sessions.end())
    {
        new_id = generateSessionID();
    }
    SessionData data;

    data.last_activity = std::time(NULL);
    data.visit_count = 1;

    _sessions[new_id] = data;
    return new_id;
}

bool Logger_manager::isValidSession(const std::string &session_id)
{
    if (session_id.empty())
        return false;

    std::map<std::string, SessionData>::iterator it = _sessions.find(session_id);
    if (it == _sessions.end())
        return false;
    time_t now = std::time(NULL);
    if (std::difftime(now, it->second.last_activity) > _session_timeout)
    {
        _sessions.erase(it);
        return false;
    }
    return true;
}

void Logger_manager::updateSession(const std::string &session_id)
{
    std::map<std::string, SessionData>::iterator it = _sessions.find(session_id);
    if (it != _sessions.end())
    {
        it->second.last_activity = std::time(NULL);
    }
}

void Logger_manager::cleanupExpiredSessions()
{
    time_t now = std::time(NULL);
    std::map<std::string, SessionData>::iterator it = _sessions.begin();
    while (it != _sessions.end())
    {
        if (std::difftime(now, it->second.last_activity) > _session_timeout)
        {
            _sessions.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

int Logger_manager::getVisitCount(const std::string &session_id)
{
    if (isValidSession(session_id))
    {
        return _sessions[session_id].visit_count;
    }
    return 0;
}

void Logger_manager::incrementVisitCount(const std::string &session_id)
{
    if (isValidSession(session_id))
    {
        _sessions[session_id].visit_count++;
    }
}