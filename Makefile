LIB=jiffy
JIFFY_VERSION=0.1.0

all:
	(cd src && make all JIFFY_VERSION=$(JIFFY_VERSION))

release: all $(LIB).1
	(cd src && make release JIFFY_VERSION=$(JIFFY_VERSION))

clean:
	(cd src && make clean)
	rm -f $(LIB).1

$(LIB).1: doc/$(APP).pod
	pod2man -r $(shell date +%Y-%m-%d) -s 1 -c 'User Commands' doc/$(LIB).pod $(APP).1
