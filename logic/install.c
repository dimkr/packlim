#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../core/pkg_list.h"
#include "../core/pkg_entry.h"
#include "../core/pkg_queue.h"
#include "../core/repo.h"
#include "../core/dep.h"
#include "../core/pkg_ops.h"
#include "../core/log.h"

#include "cleanup.h"
#include "install.h"

bool packlad_install(const char *name,
                     const char *root,
                     const char *url,
                     const char *reason,
                     const bool check_sig)
{
	char path[PATH_MAX];
	struct pkg_list list;
	struct repo repo;
	struct pkg_queue q;
	struct pkg_entry entry;
	char *next;
	bool ret = false;
	bool error = false;

	if (false == pkglist_open(&list, root))
		goto end;

	if (false == repo_open(&repo, url))
		goto close_list;

	pkgq_init(&q);
	if (false == dep_queue(&q, &list, name, root))
		goto close_repo;

	log_write(LOG_INFO, "Processing the package installation queue\n");

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
			log_write(LOG_INFO, "Aborting the installation of %s\n", name);
			error = true;
			goto free_next;
		}

		if (0 == strcmp(next, name))
			entry.reason = (char *) reason;
		else
			entry.reason = (char *) INST_REASON_DEP;
		if (false == pkg_install(path, &entry, root, check_sig)) {
			log_write(LOG_INFO, "Aborting the installation of %s\n", name);
			error = true;
			goto free_next;
		}

free_next:
		free(next);
	} while ((false == ret) && (false == error));

	pkgq_empty(&q);
	(void) packlad_cleanup(root);

close_repo:
	repo_close(&repo);

close_list:
	pkglist_close(&list);

end:
	return ret;
}
