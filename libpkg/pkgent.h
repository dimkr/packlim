#ifndef _PKGENT_H_INCLUDED
#	define _PKGENT_H_INCLUDED

#	include <stdbool.h>

#	define INST_DATA_DIR VAR_DIR"/data"

struct pkg_entry {
	char buf[512];
	char *name;
	char *ver;
	char *desc;
	char *fname;
	char *arch;
	char *deps;
	char *reason;
};

#	define INST_REASON_DEP "dependency"
#	define INST_REASON_CORE "core"
#	define INST_REASON_USER "user"

bool pkgent_register(const struct pkg_entry *entry, const char *root);
bool pkgent_unregister(const char *name, const char *root);

bool pkgent_parse(struct pkg_entry *entry);

bool pkgent_get(const char *name, struct pkg_entry *entry, const char *root);

bool pkgent_foreach(const char *root,
                    bool (*cb)(const struct pkg_entry *entry, void *arg),
                    void *arg);

#endif
