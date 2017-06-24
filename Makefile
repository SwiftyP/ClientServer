TARGET: client server

CC = gcc
CFLAGS = -Wall

client: client.o err.o
	$(CC) $(CFLAGS) $^ -o $@

server: server.o err.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean TARGETS
clean:
	rm -f server client *.o *~ *.bak
