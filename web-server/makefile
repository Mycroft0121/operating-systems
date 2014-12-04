CC = gcc
CFLAGS = -D_REENTRANT
LDFLAGS = -lpthread -pthread

all: web_server_http

web_server_http: server.o util.o
	${CC} ${LDFLAGS} -o web_server_http server.o util.o

server.o: server.c

clean:
	rm server.o web_server_http

