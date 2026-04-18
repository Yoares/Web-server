#include <iostream>
#include <exception>
#include <vector>
#include <string>

// Include our config pipeline headers
#include "../inc/config/Tokenizer.hpp"
#include "../inc/config/ConfigParser.hpp"
#include "../inc/config/ConfigValidator.hpp"

// Placeholder for the next phase
// #include "../inc/core/Webserv.hpp" 

int main(int argc, char **argv) {
    // 1. Determine the config file path
    // The subject allows a default path if no argument is provided.
    std::string config_file = "conf/default.conf";
    if (argc == 2) {
        config_file = argv[1];
    } else if (argc > 2) {
        std::cerr << "Usage: ./webserv [configuration_file]" << std::endl;
        return 1;
    }

    // 2. The Fail-Fast Try-Catch Block
    // This ensures we satisfy the 42 rule: "Your program must not crash under any circumstances"
    try {
        std::cout << "[INFO] Booting Webserv..." << std::endl;
        std::cout << "[INFO] Reading configuration from: " << config_file << std::endl;

        // --- PHASE 2: Lexical Analysis ---
        std::vector<std::string> tokens = Tokenizer::tokenize(config_file);
        std::cout << "[INFO] Tokenization complete. Found " << tokens.size() << " tokens." << std::endl;

        // --- PHASE 3: Syntax Analysis ---
        ConfigParser parser(tokens);
        std::vector<Server> servers = parser.parse();
        std::cout << "[INFO] Parsing complete. Found " << servers.size() << " server block(s)." << std::endl;

        // --- PHASE 4: Semantic Analysis ---
        ConfigValidator::validate(servers);
        std::cout << "[INFO] Validation complete. Configuration is structurally and logically sound." << std::endl;

        // --- PHASE 5: Launch the Core Engine ---
        // Webserv engine(servers);
        // engine.start(); 

        std::cout << "[INFO] (Placeholder) Server would now start listening..." << std::endl;

    } 
    catch (const std::exception& e) {
        // If the Tokenizer, Parser, or Validator throws a std::runtime_error, it lands here.
        std::cerr << "\n[FATAL ERROR] " << e.what() << std::endl;
        std::cerr << "[INFO] Server shutdown safely due to configuration errors." << std::endl;
        return 1;
    }

    return 0;
}