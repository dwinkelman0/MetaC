CC=gcc
CFLAGS=-I.
DEPS = hellomake.h
OBJ = hellomake.o hellofunc.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: main.o test_util.o grammar/util.c grammar/type.o grammar/variable.o grammar/tests/fixture.o grammar/tests/type.o grammar/tests/variable.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	find . -type f -name '*.o' -delete