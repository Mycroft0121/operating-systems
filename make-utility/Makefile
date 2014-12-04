CC=gcc

all: make4061

make4061: main.c util.c depends.o 
	$(CC) -o make4061 main.c util.c	depends.o

depends.o: depends.c depends.h target.h
	$(CC) -c depends.c

clean:
	rm -f make4061
	rm -f *.o

