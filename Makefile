
CC = gcc

all: server client

server: server.o
		$(CC) -pthread server.o -o server

client: client.o
		$(CC) client.o -o client

server.o: server.c
		$(CC) -c server.c

client.o: client.c
		$(CC) -c client.c

clean:
		rm -f *.o server client