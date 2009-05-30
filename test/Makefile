CC=cc
INCLUDES=-I../include
CFLAGS=-W -Wall -O2 $(INCLUDES)
LIBS=../src/jiffy.o
OBJS=$(shell ls *.c | sed 's/\.c/.o/')
APPS=$(shell ls *.c | sed 's/\.c//')

all: $(APPS)

clean:
	rm -f $(APPS) $(OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

cli_test: cli_test.o
	$(CC) -o cli_test $< $(LIBS)

parse_int_array: parse_int_array.o
	$(CC) -o parse_int_array $< $(LIBS)
