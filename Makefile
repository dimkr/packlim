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

include Makefile.inc

DESTDIR ?=
BIN_DIR ?= /bin
SBIN_DIR ?= /sbin
MAN_DIR ?= /usr/share/man
DOC_DIR ?= /usr/share/doc

INSTALL = install -v

all:
	cd src; $(MAKE)
	cd doc; $(MAKE)

install: all
	$(INSTALL) -D -m 755 src/packlim $(DESTDIR)/$(SBIN_DIR)/packlim
	$(INSTALL) -D -d -m 755 $(DESTDIR)/$(CONF_DIR)/packlim
	$(INSTALL) -D -m 644 doc/packlim.8 $(DESTDIR)/$(MAN_DIR)/man8/packlim.8
	$(INSTALL) -D -m 644 README $(DESTDIR)/$(DOC_DIR)/packlim/README
	$(INSTALL) -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/packlim/AUTHORS
	$(INSTALL) -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/packlim/COPYING

	$(INSTALL) -D -m 644 src/ed25519/readme.md $(DESTDIR)/$(DOC_DIR)/packlim/readme.md.ed25519
	$(INSTALL) -D -m 644 src/jimtcl/AUTHORS $(DESTDIR)/$(DOC_DIR)/packlim/AUTHORS.jimtcl
	$(INSTALL) -m 644 src/jimtcl/LICENSE $(DESTDIR)/$(DOC_DIR)/packlim/LICENSE.jimtcl

clean:
	cd doc; $(MAKE) clean
	cd src; $(MAKE) clean
