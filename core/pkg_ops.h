#ifndef _PKG_OPS_H_INCLUDED
#	define _PKG_OPS_H_INCLUDED

#	include <stdbool.h>

#	include "pkg_entry.h"

bool pkg_install(const char *path,
                 const struct pkg_entry *entry,
                 const bool check_sig);

bool pkg_remove(const char *name);

#endif
