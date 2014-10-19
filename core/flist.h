#ifndef _FLIST_H_INCLUDED
#	define _FLIST_H_INCLUDED

#	include <limits.h>
#	include <stdio.h>
#	include <stdbool.h>

struct flist {
	char path[PATH_MAX];
	FILE *fh;
};

struct flines {
	char **lines;
	unsigned int count;
};

typedef bool (*flist_iter_t)(struct flist *list,
                             const char *root,
                             bool (*cb)(const char *path, void *arg),
                             void *arg);

bool flist_open(struct flist *list,
                const char *mode,
                const char *name,
                const char *root);
void flist_close(struct flist *list);

bool flist_add(struct flist *list, const char *path);

bool flist_delete(struct flist *list);

bool flist_foreach(struct flist *list,
                   const char *root,
                   bool (*cb)(const char *path, void *arg),
                   void *arg);

bool flist_foreach_reverse(struct flist *list,
                           const char *root,
                           bool (*cb)(const char *path, void *arg),
                           void *arg);

#endif