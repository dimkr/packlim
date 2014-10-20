#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "pkg_queue.h"

void pkg_queue_init(struct pkg_queue *queue)
{
	LIST_INIT(&queue->head);
}

bool pkg_queue_push(struct pkg_queue *queue, char *name)
{
	struct pkg_task *task;

	task = (struct pkg_task *) malloc(sizeof(struct pkg_task));
	if (NULL == task)
		return false;

	task->name = name;
	task->hash = crc32(crc32(0L, Z_NULL, 0),
	                   (const Bytef *) task->name,
	                   (uInt) strlen(task->name));
	LIST_INSERT_HEAD(&queue->head, task, peers);

	return true;
}

bool pkg_queue_contains(struct pkg_queue *queue, const char *name)
{
	struct pkg_task *task;
	uLong hash;

	hash = crc32(crc32(0L, Z_NULL, 0),
	             (const Bytef *) name,
	             (uInt) strlen(name));

	LIST_FOREACH(task, &queue->head, peers) {
		if (task->hash == hash)
			return true;
	}

	return false;
}

unsigned int pkg_queue_length(struct pkg_queue *queue)
{
	struct pkg_task *task;
	unsigned int count = 0;

	LIST_FOREACH(task, &queue->head, peers)
		++count;

	return count;
}

char *pkg_queue_pop(struct pkg_queue *queue)
{
	struct pkg_task *task;
	char *name;

	task = LIST_FIRST(&queue->head);
	if (NULL == task)
		return NULL;

	name = task->name;
	LIST_REMOVE(task, peers);
	free(task);

	return name;
}

void pkg_queue_empty(struct pkg_queue *queue)
{
	char *name;

	log_write(LOG_DEBUG, "Emptying the installation queue\n");

	do {
		name = pkg_queue_pop(queue);
		if (NULL == name)
			break;
		free(name);
	} while (1);
}
