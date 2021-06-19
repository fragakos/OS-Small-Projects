CC=gcc
CFLAGS=-c -Wall -std=c99
LIBS=-lpthread -lm


all: pr1

pr1: coordinator.o sem.o
	$(CC) coordinator.o sem.o -o pr1 $(LIBS)

coordinator.o: coordinator.c
	$(CC) $(CFLAGS) coordinator.c

sem.o: sem.c
	$(CC) $(CFLAGS) sem.c

clean:
	rm -rf *o pr1
