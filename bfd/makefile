CC=gcc

CFLAGS=-c -g -std=gnu99
LIBS= 

all: bfd

bfd: bfd.o parseRc.o
	$(CC) $(LIBS) bfd.o parseRc.o -o bfd

bfd.o: bfd.c
	$(CC) $(CFLAGS) bfd.c

parseRc.o: parseRc.c
	$(CC) $(CFLAGS) parseRc.c
