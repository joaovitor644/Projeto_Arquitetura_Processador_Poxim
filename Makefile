CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC = main.c test.c
EXEC = PoximArq
OBJ = $(SRC:.c=.o)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
