#include <stdio.h>
#include <string.h>

#include "../core/pkg_list.h"
#include "../core/pkg_entry.h"
#include "../core/dep.h"
#include "../core/flist.h"
#include "../core/log.h"

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

	if (false == pkg_list_open(&list, root))
		goto end;

	if (TSTATE_OK == pkg_list_for_each(&list, list_entry))
		ret = true;

	pkg_list_close(&list);

end:
	return ret;
}

static tristate_t list_inst_entry(const struct pkg_entry *entry, void *arg)
{
	return list_entry(entry);
}

bool packlad_list_inst(const char *root)
{
	return pkg_entry_for_each(root, list_inst_entry, NULL);
}

static tristate_t list_removable(const struct pkg_entry *entry, void *arg)
{
	if (0 != strcmp(INST_REASON_USER, entry->reason))
		return TSTATE_OK;

	if (TSTATE_OK != pkg_not_required(entry->name, (const char *) arg))
		return TSTATE_OK;

	if (true == list_entry(entry))
		return TSTATE_OK;

	return TSTATE_FATAL;
}

bool packlad_list_removable(const char *root)
{
	return pkg_entry_for_each(root, list_removable, (void *) root);
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

	if (false == flist_open(&list, "r", name, root)) {
		log_write(LOG_ERR, "%s is not installed\n", name);
		goto end;
	}

	ret = flist_for_each(&list, root, list_file, NULL);

	flist_close(&list);

end:
	return ret;
}