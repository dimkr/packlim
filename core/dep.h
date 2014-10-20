#ifndef _DEP_H_INCLUDED
#	define _DEP_H_INCLUDED

#	include <stdbool.h>

#	include "types.h"
#	include "pkg_queue.h"
#	include "pkg_list.h"

struct dep_queue_params {
	struct pkg_queue *queue;
	struct pkg_list *list;
	char *root;
};

struct unneeded_iter_params {
	bool (*cb)(const struct pkg_entry *entry, void *arg);
	void *arg;
	char *root;
	unsigned int removed;
};

bool dep_queue(struct pkg_queue *queue,
               struct pkg_list *list,
               const char *name,
               const char *root);

tristate_t pkg_not_required(const char *name, const char *root);

tristate_t for_each_unneeded_pkg(const char *root,
                                 bool (*cb)(const struct pkg_entry *entry,
                                            void *arg),
                                 void *arg);

#endif