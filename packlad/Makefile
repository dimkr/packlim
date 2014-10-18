CC ?= cc
CFLAGS ?= -O2 -pipe
LDFLAGS ?= -s
PKG_CONFIG ?= pkg-config
VAR_DIR ?= /var/packlad

CFLAGS += -std=gnu99 -Wall -pedantic -DVAR_DIR=\"$(VAR_DIR)\" -I.. -I../compat
LDFLAGS += -L../ed25519 -L../libpkg
LIBS = -lpkg -led25519

CURL_CFLAGS = $(shell $(PKG_CONFIG) --cflags libcurl)
CURL_LIBS = $(shell $(PKG_CONFIG) --libs libcurl)

LIBARCHIVE_CFLAGS = $(shell $(PKG_CONFIG) --cflags libarchive)
LIBARCHIVE_LIBS = $(shell $(PKG_CONFIG) --libs libarchive)

ZLIB_CFLAGS = $(shell $(PKG_CONFIG) --cflags zlib)
ZLIB_LIBS = $(shell $(PKG_CONFIG) --libs zlib)

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
PROG = packlad

all: $(PROG)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CURL_CFLAGS) $(LIBARCHIVE_CFLAGS) $(ZLIB_CFLAGS)

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(CURL_LIBS) $(LIBARCHIVE_LIBS) $(ZLIB_LIBS)

clean:
	rm -f $(PROG) $(OBJS)
