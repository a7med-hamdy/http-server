CC = g++
CFLAGS = -pthread

main: client.o helper.o
	$(CC) $(CFLAGS) -o my_client client.o helper.o

client.o: client.cpp helper.h
	$(CC) $(CFLAGS) -c client.cpp

helper.o: helper.h helper.cpp
	$(CC) $(CFLAGS) -c helper.cpp


