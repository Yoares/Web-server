# ==============================================================================
#                                 VARIABLES
# ==============================================================================

NAME        = webserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -fsanitize=address -g -I inc

# Directories
SRC_DIR     = src
OBJ_DIR     = obj
INC_DIR     = inc

# ==============================================================================
#                               SOURCE FILES
# ==============================================================================

# Core Files
MAIN_SRC    = main.cpp
CORE_SRCS   = core/Webserv.cpp core/Connection.cpp core/methods.cpp
CONF_SRCS   = config/ServerBlock.cpp config/LocationBlock.cpp \
              config/Tokenizer.cpp config/ConfigParser.cpp config/ConfigValidator.cpp
HTTP_SRCS   = http/HttpRequest.cpp http/HttpResponse.cpp
#CGI_SRCS    = cgi/CgiHandler.cpp
UTIL_SRCS   = utils/MimeTypes.cpp 

# Combine all sources
SRCS_FILES  = $(MAIN_SRC) $(CORE_SRCS) $(CONF_SRCS) $(HTTP_SRCS) $(CGI_SRCS) $(UTIL_SRCS)

# Add src/ prefix to all files
SRCS        = $(addprefix $(SRC_DIR)/, $(SRCS_FILES))

# Translate src/*.cpp to obj/*.o
OBJS        = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# ==============================================================================
#                                  RULES
# ==============================================================================

# Default rule
all: $(NAME)

# Link the executable
$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Build successful! Run with ./$(NAME)"

# Compile object files and mirror the directory structure
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Remove object files and the obj directory
clean:
	@echo "Cleaning object files..."
	@rm -rf $(OBJ_DIR)

# Remove object files, the obj directory, and the executable
fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)

# Rebuild everything
re: fclean all

.PHONY: all clean fclean re