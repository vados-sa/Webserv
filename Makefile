NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I../include/

SRC = src/chatgbt.cpp
OBJ = $(SRC:.cpp=.o)

#pra facilitar minha vida, a gente une dps!
REQUEST_SRC = src/Request.cpp src/parsing.cpp
REQUEST_OBJ = $(REQUEST_SRC:.cpp=.o)
REQUEST_NAME = request

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)

request: $(REQUEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(REQUEST_NAME) $(REQUEST_OBJ)

clean:
	rm -f $(OBJ) $(REQUEST_OBJ)

fclean: clean
	rm -f $(NAME) $(REQUEST_NAME)

re: fclean all
