CC ?= cc
CFLAGS ?= -O2 -pipe
LDFLAGS ?= -s

CFLAGS += -std=gnu99 -Wall -pedantic

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
LIB = libed25519.a

all: $(LIB)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB): $(OBJS)
	ar rcs $@ $^

clean:
	rm -f $(LIB) $(OBJS)
