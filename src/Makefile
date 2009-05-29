LIB=libjiffy.so
AR_LIB=libjiffy.a
CC=cc
AR=ar
INCLUDES=-I../include
CFLAGS=-W -Wall -Os -g -DJIFFY_VERSION='"$(JIFFY_VERSION)"' $(INCLUDES)
LDFLAGS=-shared -Wl,-soname,$(LIB)
LIBS=-lc
OBJS=$(shell ls *.c | sed 's/\.c/.o/')

all: $(LIB) $(AR_LIB)

release: all

clean:
	rm -f $(LIB) $(AR_LIB) $(OBJS)

%.o: %.c ../include/jiffy/jiffy.h
	$(CC) -fPIC -c $(CFLAGS) $<

$(AR_LIB): $(OBJS)
	$(AR) rcs $(AR_LIB) $(OBJS)

$(LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $(LIB) $(OBJS) $(LIBS)
