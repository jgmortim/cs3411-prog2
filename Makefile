CC = gcc
FLAGS = -pedantic -Wall

all: dealer player

dealer: dealer.c
	$(CC) $(FLAGS) dealer.c -o dealer

player:
	$(CC) $(FLAGS) player.c -o player

.PHONY: clean

clean:
	rm -f dealer player *.o
