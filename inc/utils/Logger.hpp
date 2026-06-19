#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <map>
#include <ctime>
#include <cstdlib>

struct SessionData
{
	time_t last_activity;
	int visit_count;
};

class Logger_manager
{
	private:
		std::map<std::string, SessionData> _sessions;
		int _session_timeout;

		std::string generateSessionID();

	public:
		Logger_manager();
		~Logger_manager();

		std::string createSession();
		bool isValidSession(const std::string &session_id);
		void updateSession(const std::string &session_id);
		void cleanupExpiredSessions();

		int getVisitCount(const std::string &session_id);
		void incrementVisitCount(const std::string &session_id);
};

#endif