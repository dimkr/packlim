#ifndef _PKGLIST_H_INCLUDED
#	define _PKGLIST_H_INCLUDED

#	include <stdio.h>
#	include <stdbool.h>

#	include "pkgent.h"

#	define PKG_LIST_FILE_NAME "pkg_list"
#	define PKG_LIST_PATH VAR_DIR"/"PKG_LIST_FILE_NAME

struct pkg_list {
	FILE *fh;
};

bool pkglist_open(struct pkg_list *list, const char *root);
void pkglist_close(struct pkg_list *list);

bool pkglist_get(struct pkg_list *list,
                 struct pkg_entry *entry,
                 const char *name);

bool pkglist_foreach(struct pkg_list *list,
                     bool (*cb)(const struct pkg_entry *entry));

#endif