
CC = gcc

all: sserver bserver tserver

sserver: sserver.o
		$(CC) -pthread sserver.o -o sserver

bserver: bserver.o
		$(CC) -pthread bserver.o -o bserver

tserver: tserver.o
		$(CC) -pthread tserver.o -o tserver

sserver.o: sserver.c
		$(CC) -c sserver.c

bserver.o: bserver.c
		$(CC) -c bserver.c

tserver.o: tserver.c
		$(CC) -c tserver.c

clean:
		rm -f *.o sserver bserver tserver