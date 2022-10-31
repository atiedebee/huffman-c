SRC = main.c encode.c decode.c
OBJ = $(addprefix bin/, $(SRC:.c=.o))

CC = gcc-11
FLAGS = -pedantic-errors -Wpedantic -Wall -Wextra -O2 -flto -Werror

default: $(OBJ)
	$(CC) $(OBJ) $(FLAGS)

bin/%.o:%.c
	$(CC) -c $^ -o $@ $(FLAGS)

.PHONY: clean

clean:
	rm $(OBJ)
	rm a.out
