# this file is part of packlim.
#
# Copyright (c) 2016 Dima Krasner
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

include ../Makefile.inc

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

JIMTCL_CFLAGS = -I./jimtcl
JIMTCL_LIBS = -L./jimtcl -ljim

ED25519_CFLAGS = -I./ed25519/src

CURL_CFLAGS = $(shell $(PKG_CONFIG) --cflags libcurl)
CURL_LIBS = $(shell $(PKG_CONFIG) --libs libcurl)

LIBARCHIVE_CFLAGS = $(shell $(PKG_CONFIG) --cflags libarchive)
LIBARCHIVE_LIBS = $(shell $(PKG_CONFIG) --libs libarchive)

SRCS = $(wildcard *.c) $(wildcard ed25519/src/*.c)
OBJECTS = $(SRCS:.c=.o)

all: packlim

jimtcl/jim-config.h:
	cd jimtcl; CC=$(CC) CFLAGS="$(CFLAGS)" ./configure --with-ext="aio file glob package stdlib tclcompat namespace history clock" --without-ext=default

jimtcl/libjim.a: jimtcl/jim-config.h
	cd jimtcl; $(MAKE) libjim.a

jim-tar.o: jim-tar.c jimtcl/jim-config.h
	$(CC) -c -o $@ $< $(CFLAGS) $(JIMTCL_CFLAGS) $(LIBARCHIVE_CFLAGS)

jim-curl.o: jim-curl.c jimtcl/jim-config.h
	$(CC) -c -o $@ $< $(CFLAGS) $(JIMTCL_CFLAGS) $(CURL_CFLAGS)

jim-ed25519.o: jim-ed25519.c jimtcl/jim-config.h
	$(CC) -c -o $@ $< $(CFLAGS) $(JIMTCL_CFLAGS) $(ED25519_CFLAGS)

%.inc: %.tcl
	sed -e s~'^\t*'~~g \
	    -e s~'\\'~'\\\\'~g \
	    -e s~'"'~'\\"'~g \
	    -e s~'^'~'"'~g \
	    -e s~'$$'~'\\n" \\'~g \
	    -e s~@CONF_DIR@~$(CONF_DIR)~g \
	    -e s~@VAR_DIR@~$(VAR_DIR)~g \
	    $^ | \
	grep -v \
	     -e ^\"\# \
	     -e '^"\\n" \\' > $@; \
	echo \"\" >> $@

jim-packlim.o: jim-packlim.c packlim.inc jimtcl/jim-config.h
	$(CC) -c -o $@ $< $(CFLAGS) $(JIMTCL_CFLAGS)

main.o: main.c main.inc jimtcl/jim-config.h
	$(CC) -c -o $@ $< $(CFLAGS) $(JIMTCL_CFLAGS)

%.o: %.c jimtcl/jim-config.h
	$(CC) -c -o $@ $< $(CFLAGS) $(JIMTCL_CFLAGS)

packlim: jimtcl/libjim.a $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) $(CURL_LIBS) $(LIBARCHIVE_LIBS) $(JIMTCL_LIBS) $(LIBS)

clean:
	cd jimtcl; $(MAKE) clean
	rm -f packlim main.inc packlim.inc $(OBJECTS)
