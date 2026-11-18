NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -std=c++98 -I src/includes

SRC_DIR = src
SRCS = $(SRC_DIR)/main.cpp \
	$(SRC_DIR)/core/server/server.cpp \
	$(SRC_DIR)/core/server/init.cpp \
	$(SRC_DIR)/core/server/Server_Run.cpp \
	$(SRC_DIR)/core/server/EchoHandler.cpp \
	$(SRC_DIR)/http/DataHandler.cpp

OBJ_DIR = obj
OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
