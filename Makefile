CC=gcc
CFLAGS=-I.
DEPS = hellomake.h
OBJ = hellomake.o hellofunc.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: tests/variables.o decl.o expr.o main.o util.o
	$(CC) -o $@ $^ $(CFLAGS)