CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS=-lpthread
SOURCE=proj2.c

all:
	$(CC) $(CFLAGS) $(SOURCE) -o proj2 $(LFLAGS)
