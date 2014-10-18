#include <stdio.h>
#include <string.h>

#include <libpkg/pkglist.h>
#include <libpkg/pkgent.h>
#include <libpkg/dep.h>
#include <libpkg/flist.h>

#include "list.h"

static bool list_entry(const struct pkg_entry *entry)
{
	if (0 > printf("%s|%s|%s\n",
	               entry->name,
	               entry->ver,
	               entry->desc))
		return false;

	return true;
}

bool packlad_list_avail(const char *root)
{
	struct pkg_list list;
	bool ret = false;

	if (false == pkglist_open(&list, root))
		goto end;

	ret = pkglist_foreach(&list, list_entry);

	pkglist_close(&list);

end:
	return ret;
}

static bool list_inst_entry(const struct pkg_entry *entry, void *arg)
{
	return list_entry(entry);
}

bool packlad_list_inst(const char *root)
{
	return pkgent_foreach(root, list_inst_entry, NULL);
}

static bool list_removable(const struct pkg_entry *entry, void *arg)
{
	if (0 != strcmp(INST_REASON_USER, entry->reason))
		return true;

	if (true == pkg_required(entry->name, (const char *) arg))
		return true;

	return list_entry(entry);
}

bool packlad_list_removable(const char *root)
{
	return pkgent_foreach(root, list_removable, (void *) root);
}

static bool list_file(const char *path, void *arg)
{
	if (0 > printf("%s\n", path))
		return false;

	return true;
}

bool packlad_list_files(const char *name, const char *root)
{
	struct flist list;
	bool ret = false;

	if (false == flist_open(&list, "r", name, root))
		goto end;

	ret = flist_foreach(&list, root, list_file, NULL);

	flist_close(&list);

end:
	return ret;
}