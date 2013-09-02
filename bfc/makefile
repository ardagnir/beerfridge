CC=gcc

CFLAGS=-c -g -std=gnu99 -O3

all: bfc

bfc: bfc.o
	$(CC) $(LIBS) bfc.o -o bfc

bfc.o: bfc.c
	$(CC) $(CFLAGS) bfc.c
