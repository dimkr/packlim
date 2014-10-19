#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "pkg_entry.h"
#include "flist.h"

bool flist_open(struct flist *list,
                const char *mode,
                const char *name,
                const char *root)
{
	(void) sprintf(list->path, "%s"INST_DATA_DIR"/%s/files", root, name);
	list->fh = fopen(list->path, mode);
	if (NULL == list->fh)
		return false;

	return true;
}

void flist_close(struct flist *list)
{
	(void) fclose(list->fh);
}

bool flist_add(struct flist *list, const char *path)
{
	if (0 < fprintf(list->fh, "%s\n", path))
		return false;

	return true;
}

bool flist_delete(struct flist *list)
{
	if (-1 == unlink(list->path))
		return false;

	return true;
}

static bool flist_foreach_internal(struct flist *list,
                                   const char *root,
                                   bool (*cb)(const char *path, void *arg),
                                   void *arg)
{
	char path[PATH_MAX];
	size_t len;
	bool ret = false;

	rewind(list->fh);

	do {
		if (NULL == fgets(path, sizeof(path), list->fh)) {
			if (0 != feof(list->fh))
				ret = true;
			break;
		}

		len = strlen(path) - 1;
		if ('\n' == path[len])
			path[len] = '\0';

		if (false == cb(path, arg))
			break;
	} while (1);

end:
	return ret;
}

static bool wrap_iter(const flist_iter_t iter,
                      struct flist *list,
                      const char *root,
                      bool (*cb)(const char *path, void *arg),
                      void *arg)
{
	char wd[PATH_MAX];
	bool ret = false;

	if (NULL == getcwd(wd, sizeof(wd)))
		goto end;

	if (-1 == chdir(root))
		goto end;

	ret = iter(list, root, cb, arg);

	if (-1 == chdir(wd))
		ret = false;

end:
	return ret;
}

bool flist_foreach(struct flist *list,
                   const char *root,
                   bool (*cb)(const char *path, void *arg),
                   void *arg)
{
	return wrap_iter(flist_foreach_internal, list, root, cb, arg);
}

static bool append_line(const char *path, void *arg)
{
	struct flines *lines = (struct flines *) arg;
	struct flines nlines;

	nlines.count = 1 + lines->count;
	nlines.lines = realloc(lines->lines, sizeof(char *) * nlines.count);
	if (NULL == nlines.lines) {
		if (NULL != lines->lines)
			free(lines->lines);
		return false;
	}

	nlines.lines[lines->count] = strdup(path);
	lines->lines = nlines.lines;
	if (NULL == nlines.lines[lines->count])
		return false;
	lines->count = nlines.count;

	return true;
}

static bool flist_foreach_reverse_internal(
                                        struct flist *list,
                                        const char *root,
                                        bool (*cb)(const char *path, void *arg),
                                        void *arg)
{
	struct flines lines;
	int i;
	bool ret = false;

	lines.lines = NULL;
	lines.count = 0;

	if (false == flist_foreach(list, root, append_line, (void *) &lines))
		goto end;

	for (i = lines.count - 1; 0 <= i; --i) {
		if (false == cb(lines.lines[i], arg))
			goto free_lines;
	}

	ret = true;

free_lines:
	for (i = 0; lines.count > i; ++i)
		free(lines.lines[i]);
	if (NULL != lines.lines)
		free(lines.lines);

end:
	return ret;
}

bool flist_foreach_reverse(struct flist *list,
                           const char *root,
                           bool (*cb)(const char *path, void *arg),
                           void *arg)
{
	return wrap_iter(flist_foreach_reverse_internal, list, root, cb, arg);
}
