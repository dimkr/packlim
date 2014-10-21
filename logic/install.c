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
	unsigned int count;
	int len;
	bool ret = false;
	bool error = false;

	if (false == pkg_list_open(&list))
		goto end;

	log_write(LOG_INFO, "Building the package queue\n");

	pkg_queue_init(&q);
	if (false == dep_queue(&q, &list, name))
		goto close_list;

	count = pkg_queue_length(&q);
	if (0 == count) {
		ret = true;
		goto close_list;
	}

	if (false == repo_open(&repo, url))
		goto empty_queue;

	log_write(LOG_INFO, "Processing the package queue (%u packages)\n", count);

	do {
		next = pkg_queue_pop(&q);
		if (NULL == next) {
			ret = true;
			goto free_next;
		}

		if (TSTATE_OK != pkg_list_get(&list, &entry, next))
			break;

		len = snprintf(path, sizeof(path), PKG_ARCHIVE_DIR"/%s", entry.fname);
		if ((sizeof(path) <= len) || (0 > len)) {
			error = true;
			goto free_next;
		}

		if (false == repo_fetch(&repo, entry.fname, path)) {
			error = true;
			goto free_next;
		}

		if (0 == strcmp(next, name))
			entry.reason = (char *) reason;
		else
			entry.reason = (char *) INST_REASON_DEP;
		if (false == pkg_install(path, &entry, check_sig)) {
			log_write(LOG_ERR, "Cannot install %s\n", name);
			error = true;
			goto free_next;
		}

free_next:
		free(next);
	} while ((false == ret) && (false == error));

	(void) packlad_cleanup();

	repo_close(&repo);

empty_queue:
	pkg_queue_empty(&q);

close_list:
	pkg_list_close(&list);

end:
	return ret;
}
