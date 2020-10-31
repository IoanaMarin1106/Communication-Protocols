# Protocoale de comunicatii:
# Tema2
# Makefile

CFLAGS = -Wall -g

all: server subscriber

# Compileaza server.c
server: server.c

# Compileaza subscriber.c
subscriber: subscriber.c -lm

.PHONY: clean run_server run_client

clean:
	rm -f server subscriber
