#ifndef _DEP_H_INCLUDED
#	define _DEP_H_INCLUDED

#	include <stdbool.h>

#	include "pkgq.h"
#	include "pkglist.h"

struct dep_queue_params {
	struct pkg_queue *queue;
	struct pkg_list *list;
	char *root;
};

struct required_params {
	char *name;
	bool result;
};

struct unrequired_iter_params {
	bool (*cb)(const struct pkg_entry *entry, void *arg);
	void *arg;
	char *root;
	unsigned int removed;
};

bool dep_foreach(const char *deps,
                 bool (*cb)(const char *dep, void *arg),
                 void *arg);

bool dep_queue(struct pkg_queue *queue,
               struct pkg_list *list,
               const char *name,
               const char *root);

bool pkg_required(const char *name, const char *root);

bool unrequired_foreach(const char *root,
                        bool (*cb)(const struct pkg_entry *entry,
                                   void *arg),
                        void *arg);

#endif