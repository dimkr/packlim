#ifndef _PKG_LIST_H_INCLUDED
#	define _PKG_LIST_H_INCLUDED

#	include <stdio.h>
#	include <stdbool.h>

#	include "pkg_entry.h"

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
