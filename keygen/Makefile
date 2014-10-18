CC ?= cc
CFLAGS ?= -O2 -pipe
LDFLAGS ?= -s

CFLAGS += -std=gnu99 -Wall -pedantic -I..
LDFLAGS += -L../ed25519
LIBS = -led25519

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
PROG = keygen

all: $(PROG)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(PROG) $(OBJS)