all: user1 user2
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99

user1: user1.o librsocket.a
	$(CC) $(CFLAGS) -o user1 user1.o -L. -lrsocket -lpthread
user2: user2.o librsocket.a
	$(CC) $(CFLAGS) -o user2 user2.o -L. -lrsocket -lpthread

user1.o: user1.c
	$(CC) $(CFLAGS) -c user1.c -o user1.o

user2.o: user2.c
	$(CC) $(CFLAGS)  -c user2.c -o user2.o

clean: 
	rm -f user1 user2 user1.o user2.o