CC = gcc
CFLAGS = -Wall -Wextra -g -O3

OUT = lz78
GERADOR = fazarquivo

SRC = main.c lz78.c trie.c
OBJ = $(SRC:.c=.o)

GERADOR_SRC = fazarquivo.c
GERADOR_OBJ = $(GERADOR_SRC:.c=.o)


.PHONY: all clean

all: $(OUT) $(GERADOR)


$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(OUT)


$(GERADOR): $(GERADOR_OBJ)
	$(CC) $(CFLAGS) $(GERADOR_OBJ) -o $(GERADOR)


main.o: main.c lz78.h

lz78.o: lz78.c lz78.h trie.h

trie.o: trie.c trie.h

fazarquivo.o: fazarquivo.c


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) $(GERADOR_OBJ) $(OUT) $(GERADOR)
	rm -f dados/*.lz78 dados/*-recuperado.txt