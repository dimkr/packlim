#include "../core/pkg_ops.h"
#include "../core/dep.h"

#include "cleanup.h"

static bool remove_unneeded(const struct pkg_entry *entry, void *arg)
{
	return pkg_remove(entry->name, (const char *) arg);
}

bool packlad_cleanup(const char *root)
{
	return for_each_unneeded_pkg(root, remove_unneeded, (void *) root);
}
