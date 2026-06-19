#include <iostream>
#include <exception>
#include <vector>
#include <string>

#include "../inc/config/Tokenizer.hpp"
#include "../inc/config/ConfigParser.hpp"
#include "../inc/config/ConfigValidator.hpp"
#include "../inc/utils/Logger.hpp"

#include "../inc/core/Webserv.hpp" 

#include <csignal>

bool g_server_running = true;

void handle_sigint(int sig)
{
    (void)sig;
    std::cout << "\n[INFO] SIGINT (Ctrl+C) received. Shutting down gracefully..." << std::endl;
    g_server_running = false;
}

int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
    std::string config_file = "conf/default.conf";
    if (argc == 2) {
        config_file = argv[1];
    } else if (argc > 2) {
        std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
        return 1;
    }
    try {
        std::cout << "[INFO] Booting Webserv..." << std::endl;
        std::cout << "[INFO] Reading configuration from: " << config_file << std::endl;
        std::vector<std::string> tokens = Tokenizer::tokenize(config_file);
        std::cout << "[INFO] Tokenization complete. Found " << tokens.size() << " tokens." << std::endl;
        ConfigParser parser(tokens);
        std::vector<Server> servers = parser.parse();
        std::cout << "[INFO] Parsing complete. Found " << servers.size() << " server block(s)." << std::endl;

        ConfigValidator::validate(servers);
        std::cout << "[INFO] Validation complete. Configuration is structurally and logically sound." << std::endl;
		signal(SIGINT, handle_sigint);
		Logger_manager session_manager;
		Webserv engine(servers, session_manager);
		engine.run();
    } 
    catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR] " << e.what() << std::endl;
        std::cerr << "[INFO] Server shutdown safely due to configuration errors." << std::endl;
        return 1;
    }

    return 0;
}