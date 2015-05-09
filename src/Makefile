CC ?= cc
PKG_CONFIG ?= pkg-config

CFLAGS ?= -O2 -pipe
LIBS ?=
LDFLAGS ?= -Wl,-s

VERSION = 1.0

CFLAGS += -std=gnu99 \
          -Wall \
          -pedantic \
          -DVERSION=\"$(VERSION)\"

LIBS += -ljim -led25519

CURL_CFLAGS = $(shell $(PKG_CONFIG) --cflags libcurl)
CURL_LIBS = $(shell $(PKG_CONFIG) --libs libcurl)

LIBARCHIVE_CFLAGS = $(shell $(PKG_CONFIG) --cflags libarchive)
LIBARCHIVE_LIBS = $(shell $(PKG_CONFIG) --libs libarchive)

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)
HEADERS = $(wildcard *.h)
TCLS = $(wildcard *.tcl)

all: packlim

packlim.inc: $(TCLS)
	for tcl in $^; do \
		sed -e s~'\\'~'\\\\'~g -e s~'"'~'\\"'~g -e s~'^'~'"'~g -e s~'$$'~'\\n" \\'~g $$tcl >> $@; \
		echo '"\\n" \\' >> $@; \
	done
	echo \"\" >> $@

packlim.o: packlim.c packlim.inc $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

repo.o: repo.c repo.h
	$(CC) -c -o $@ $< $(CFLAGS) $(CURL_CFLAGS)

log.o: log.c log.h
	$(CC) -c -o $@ $< $(CFLAGS)

package.o: package.c package.h
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBARCHIVE_CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

packlim: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CURL_LIBS) $(LIBARCHIVE_LIBS) $(LIBS)

clean:
	rm -f packlim $(OBJECTS) packlim.inc
