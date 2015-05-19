CC ?= cc
PKG_CONFIG ?= pkg-config

CFLAGS ?= -O2 -pipe
LIBS ?=
LDFLAGS ?= -Wl,-s

VERSION = 1.0

CFLAGS += -std=gnu99 \
          -Wall \
          -pedantic \
          -Wno-overlength-strings \
          -DVERSION=\"$(VERSION)\"

LIBS += -ljim -led25519

CURL_CFLAGS = $(shell $(PKG_CONFIG) --cflags libcurl)
CURL_LIBS = $(shell $(PKG_CONFIG) --libs libcurl)

LIBARCHIVE_CFLAGS = $(shell $(PKG_CONFIG) --cflags libarchive)
LIBARCHIVE_LIBS = $(shell $(PKG_CONFIG) --libs libarchive)

SRCS = $(wildcard *.c)
OBJECTS = $(SRCS:.c=.o)

all: packlim

jim-tar.o: jim-tar.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBARCHIVE_CFLAGS)

jim-curl.o: jim-curl.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CURL_CFLAGS)

%.inc: %.tcl
	sed -e s~'\\'~'\\\\'~g -e s~'"'~'\\"'~g -e s~'^'~'"'~g -e s~'$$'~'\\n" \\'~g $^ >> $@; \
	echo '"\\n" \\' >> $@; \
	echo \"\" >> $@

jim-packlim.o: jim-packlim.c packlim.inc
	$(CC) -c -o $@ $< $(CFLAGS)

main.o: main.c main.inc
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

packlim: $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CURL_LIBS) $(LIBARCHIVE_LIBS) $(LIBS)

clean:
	rm -f packlim main.inc packlim.inc $(OBJECTS)
