#include <stdlib.h>
#include <stdio.h>

#include <libpkg/pkglist.h>
#include <libpkg/pkgent.h>
#include <libpkg/pkgq.h>
#include <libpkg/repo.h>
#include <libpkg/dep.h>
#include <libpkg/ops.h>

#include "cleanup.h"
#include "install.h"

bool packlad_install(const char *name,
                      const char *root,
                      const char *url,
                      const char *reason)
{
	char path[PATH_MAX];
	struct pkg_list list;
	struct repo repo;
	struct pkg_queue q;
	struct pkg_entry entry;
	char *next;
	bool ret = false;
	bool error = false;
	unsigned int i;

	if (false == pkglist_open(&list, root))
		goto end;

	if (false == repo_open(&repo, url))
		goto close_list;

	pkgq_init(&q);
	if (false == dep_queue(&q, &list, name, root))
		goto close_repo;

	i = 0;
	do {
		next = pkgq_pop(&q);
		if (NULL == next) {
			ret = true;
			goto free_next;
		}

		if (false == pkglist_get(&list, &entry, next))
			break;

		(void) sprintf(path, "%s"PKG_ARCHIVE_DIR"/%s", root, entry.fname);
		if (false == repo_fetch(&repo, entry.fname, path)) {
			error = true;
			goto free_next;
		}

		if (0 == i)
			entry.reason = (char *) reason;
		else
			entry.reason = (char *) INST_REASON_DEP;
		if (false == pkg_install(path, &entry, root)) {
			error = true;
			goto free_next;
		}

		++i;

free_next:
		free(next);
	} while ((false == ret) && (false == error));

	(void) packlad_cleanup(root);

close_repo:
	repo_close(&repo);

close_list:
	pkglist_close(&list);

end:
	return ret;
}