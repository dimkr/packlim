DESTDIR ?=
BIN_DIR ?= /bin
SBIN_DIR ?= /sbin
MAN_DIR ?= /usr/share/man
DOC_DIR ?= /usr/share/doc
CONF_DIR ?= /etc

INSTALL = install -v

all:
	cd src; $(MAKE)

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
	cd src; $(MAKE) clean
