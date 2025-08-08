NAME = webserv

CXX = c++
CXXFLAGS = -g -O0 -Wall -Wextra -Werror -std=c++98 -Iinclude/

SRC = src/main.cpp src/Server.cpp src/Client.cpp src/HttpMessage.cpp src/Request.cpp src/Response.cpp src/Config.cpp src/ConfigParser.cpp src/ServerConfig.cpp src/LocationConfig.cpp
OBJ_DIR = obj
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)


# Colors
GREEN = \033[0;32m
YELLOW = \033[0;33m
RED = \033[0;31m
NC = \033[0m # No Color

all: $(NAME)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(NC)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(NAME): $(OBJ)
	@echo "$(YELLOW)Linking $(NAME)...$(NC)"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)
	@echo "$(GREEN)✅ $(NAME) is ready to run!$(NC)"

clean:
	@echo "$(RED)Cleaning object files...$(NC)"
	rm -f $(OBJ) $(REQUEST_OBJ)
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "$(RED)Cleaning executable...$(NC)"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
