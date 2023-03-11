CC = g++
CFLAGS = -pthread


main: server.o
	$(CC) $(CFLAGS) -o my_server server.o

server.o: server.cpp
	$(CC) $(CFLAGS) -c server.cpp