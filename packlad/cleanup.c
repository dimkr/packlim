#include <libpkg/ops.h>
#include <libpkg/dep.h>

#include "cleanup.h"

static bool remove_unneeded(const struct pkg_entry *entry, void *arg)
{
	return pkg_remove(entry->name, (const char *) arg);
}

bool packlad_cleanup(const char *root)
{
	return unrequired_foreach(root, remove_unneeded, (void *) root);
}
