NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = chatgbt.cpp
OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJ)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all