DESTDIR ?=
BIN_DIR ?= /bin
SBIN_DIR ?= /sbin
MAN_DIR ?= /usr/share/man
DOC_DIR ?= /usr/share/doc

INSTALL = install -v

all:
	cd src; $(MAKE)

install: all
	$(INSTALL) -D -m 755 src/packlim $(DESTDIR)/$(SBIN_DIR)/packlim
	$(INSTALL) -D -m 755 src/tar2pkg $(DESTDIR)/$(BIN_DIR)/tar2pkg
	$(INSTALL) -D -m 644 doc/packlim.8 $(DESTDIR)/$(MAN_DIR)/man8/packlim.8
	$(INSTALL) -D -m 644 doc/tar2pkg.1 $(DESTDIR)/$(MAN_DIR)/man1/tar2pkg.1
	$(INSTALL) -D -m 644 README $(DESTDIR)/$(DOC_DIR)/packlim/README
	$(INSTALL) -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/packlim/AUTHORS
	$(INSTALL) -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/packlim/COPYING

clean:
	cd src; $(MAKE) clean
