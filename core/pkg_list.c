#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "log.h"
#include "pkg_list.h"

bool pkglist_open(struct pkg_list *list, const char *root)
{
	char path[PATH_MAX];

	(void) sprintf(path, "%s"PKG_LIST_PATH, root);

	list->fh = fopen(path, "r");
	if (NULL == list->fh)
		return false;

	return true;
}

void pkglist_close(struct pkg_list *list)
{
	(void) fclose(list->fh);
}

bool pkglist_get(struct pkg_list *list,
                 struct pkg_entry *entry,
                 const char *name)
{
	log_write(LOG_DEBUG, "Searching the package list for %s\n", name);

	rewind(list->fh);

	do {
		if (NULL == fgets(entry->buf, sizeof(entry->buf), list->fh)) {
			if (0 != feof(list->fh))
				break;
			else {
				log_write(LOG_ERR, "Failed to read the package list\n");
				return false;
			}
		}

		if (false == pkgent_parse(entry)) {
			log_write(LOG_ERR, "Failed to parse the package list\n");
			return false;
		}

		if (0 == strcmp(name, entry->name)) {
			log_write(LOG_INFO, "Found %s, version %s\n", name, entry->ver);
			return true;
		}
	} while (1);

	log_write(LOG_ERR, "Failed to locate %s in the package list\n", name);
	return false;
}

bool pkglist_foreach(struct pkg_list *list,
                     bool (*cb)(const struct pkg_entry *entry))
{
	struct pkg_entry entry;

	rewind(list->fh);

	do {
		if (NULL == fgets(entry.buf, sizeof(entry.buf), list->fh)) {
			if (0 != feof(list->fh))
				break;
			else {
				log_write(LOG_ERR, "Failed to read the package list\n");
				return false;
			}
		}

		if (false == pkgent_parse(&entry)) {
			log_write(LOG_ERR, "Failed to parse the package list\n");
			return false;
		}

		if (false == cb(&entry))
			return false;
	} while (1);

	return true;
}