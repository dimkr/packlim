DESTDIR ?=
BIN_DIR ?= /bin
DOC_DIR ?= /usr/share/doc
VAR_DIR ?= /var/packlad

INSTALL = install -v

all: dir2pkg/dir2pkg packlad/packlad

ed25519/libed25519.a:
	cd ed25519; $(MAKE)

keygen/keygen: ed25519/libed25519.a
	cd keygen; $(MAKE)

libpkg/pub_key libpkg/pub_key.h libpkg/priv_key libpkg/priv_key.h: keygen/keygen
	./keygen/keygen libpkg/pub_key \
	                libpkg/pub_key.h \
	                libpkg/priv_key \
	                libpkg/priv_key.h

libpkg/libpkg.a: ed25519/libed25519.a libpkg/pub_key.h
	cd libpkg; $(MAKE)

packlad/packlad: libpkg/libpkg.a
	cd packlad; $(MAKE)

pkgsign/pkgsign: libpkg/libpkg.a
	cd pkgsign; $(MAKE)

dir2pkg/dir2pkg: pkgsign/pkgsign
	cd dir2pkg; $(MAKE)

install: all
	$(INSTALL) -D -m 755 pkgsign/pkgsign $(DESTDIR)/$(BIN_DIR)/pkgsign
	$(INSTALL) -m 755 dir2pkg/dir2pkg $(DESTDIR)/$(BIN_DIR)/dir2pkg
	$(INSTALL) -m 755 packlad/packlad $(DESTDIR)/$(BIN_DIR)/packlad
	$(INSTALL) -D -m 644 README $(DESTDIR)/$(DOC_DIR)/packlad/README
	$(INSTALL) -m 644 AUTHORS $(DESTDIR)/$(DOC_DIR)/packlad/AUTHORS
	$(INSTALL) -m 644 COPYING $(DESTDIR)/$(DOC_DIR)/packlad/COPYING
	$(INSTALL) -d -m 755 $(DESTDIR)/$(VAR_DIR)
	$(INSTALL) -d -m 755 $(DESTDIR)/$(VAR_DIR)/data
	$(INSTALL) -d -m 755 $(DESTDIR)/$(VAR_DIR)/archive


clean:
	cd dir2pkg; $(MAKE) clean
	cd pkgsign; $(MAKE) clean
	cd libpkg; $(MAKE) clean
	rm -f libpkg/priv_key.h libpkg/priv_key libpkg/pub_key.h libpkg/pub_key
	cd keygen; $(MAKE) clean
	cd ed25519; $(MAKE) clean
	cd packlad; $(MAKE) clean
