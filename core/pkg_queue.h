#ifndef _PKG_QUEUE_H_INCLUDED
#	define _PKG_QUEUE_H_INCLUDED

#	include <stdbool.h>

#	include "../compat/queue.h"

#	include <zlib.h>

struct pkg_task {
	char *name;
	uLong hash;
	LIST_ENTRY(pkg_task) peers;
};

struct pkg_queue {
    LIST_HEAD(pkg_task_list, pkg_task) head;
};

void pkgq_init(struct pkg_queue *queue);
bool pkgq_push(struct pkg_queue *queue, char *name);
bool pkgq_contains(struct pkg_queue *queue, const char *name);
char *pkgq_pop(struct pkg_queue *queue);
void pkgq_empty(struct pkg_queue *queue);

#endif
