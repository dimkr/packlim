#ifndef _OPS_H_INCLUDED
#	define _OPS_H_INCLUDED

#	include <stdbool.h>

#	include "pkgent.h"

bool pkg_install(const char *path,
                 const struct pkg_entry *entry,
                 const char *root);

bool pkg_remove(const char *name, const char *root);

#endif