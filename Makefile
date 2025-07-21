NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude/ -g

SRC = src/main.cpp src/Server.cpp src/Client.cpp src/HttpMessage.cpp src/Request.cpp src/Response.cpp
OBJ_DIR = obj
OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

#pra facilitar minha vida, a gente une dps!
REQUEST_SRC = src/Request.cpp src/requestMain.cpp src/Response.cpp src/HttpMessage.cpp
REQUEST_OBJ = $(REQUEST_SRC:.cpp=.o)
REQUEST_NAME = request

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

r: $(REQUEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(REQUEST_NAME) $(REQUEST_OBJ)

clean:
	rm -f $(OBJ) $(REQUEST_OBJ)

re: fclean all
	@echo "$(RED)Cleaning object files...$(NC)"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "$(RED)Cleaning executable...$(NC)"
	@rm -f $(NAME)

.PHONY: all clean fclean re
