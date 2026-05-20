CC = gcc
CFLAGS = -Wall -Wextra -g -O3

SRC = main.c
OBJ = $(SRC:.c=.o)

OUT = lz78

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(OUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(OUT)
