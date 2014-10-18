CC ?= cc
CFLAGS ?= -O2 -pipe
LDFLAGS ?= -s
PKG_CONFIG ?= pkg-config
VAR_DIR ?= /var/packlad

CFLAGS += -std=gnu99 -Wall -pedantic -I.. -DVAR_DIR=\"$(VAR_DIR)\"
LIBARCHIVE_CFLAGS = $(shell $(PKG_CONFIG) --cflags libarchive)

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIB = libpkg.a

all: $(LIB)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBARCHIVE_CFLAGS)

$(LIB): $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(LIB) $(OBJS)
