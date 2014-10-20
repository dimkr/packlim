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

void pkg_queue_init(struct pkg_queue *queue);
bool pkg_queue_push(struct pkg_queue *queue, char *name);
bool pkg_queue_contains(struct pkg_queue *queue, const char *name);
unsigned int pkg_queue_length(struct pkg_queue *queue);
char *pkg_queue_pop(struct pkg_queue *queue);
void pkg_queue_empty(struct pkg_queue *queue);

#endif
