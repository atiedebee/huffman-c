SRC = main.c encode.c decode.c
OBJ = $(addprefix bin/, $(SRC:.c=.o))

CC = gcc
FLAGS = -pedantic-errors -Wpedantic -Wall -Wextra -O2 -flto

default: $(OBJ)
	$(CC) $(OBJ) $(FLAGS)

bin/%.o:%.c
	@mkdir -p $(dir $@)
	$(CC) -c $^ -o $@ $(FLAGS)

.PHONY: clean

clean:
	rm $(OBJ)
	rm a.out
