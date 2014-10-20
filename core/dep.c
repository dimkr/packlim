#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "dep.h"

static tristate_t for_each_dep(const char *deps,
                               tristate_t (*cb)(const char *dep, void *arg),
                               void *arg)
{
	char *copy;
	char *dep;
	char *pos;
	tristate_t ret = TSTATE_FATAL;

	if (0 == strlen(deps)) {
		ret = TSTATE_OK;
		goto end;
	}

	copy = strdup(deps);
	if (NULL == copy)
		goto end;

	dep = strtok_r(copy, " ", &pos);
	while (NULL != dep) {
		ret = cb(dep, arg);
		if (TSTATE_OK != ret)
			goto free_copy;
		dep = strtok_r(NULL, " ", &pos);
	}

	ret = TSTATE_OK;

free_copy:
	free(copy);

end:
	return ret;
}

static tristate_t dep_recurse(const char *dep, void *arg)
{
	struct dep_queue_params *params = (struct dep_queue_params *) arg;

	if (true == dep_queue(params->queue, params->list, dep, params->root))
		return TSTATE_OK;

	return TSTATE_FATAL;
}

bool dep_queue(struct pkg_queue *queue,
               struct pkg_list *list,
               const char *name,
               const char *root)
{
	struct pkg_entry entry;
	struct dep_queue_params params;
	char *copy;

	if (true == pkg_queue_contains(queue, name)) {
		log_write(LOG_DEBUG, "%s is already queued for installation\n", name);
		return true;
	}

	switch (pkg_entry_get(name, &entry, root)) {
		case TSTATE_OK:
			log_write(LOG_WARN, "%s is already installed\n", name);
			return true;

		case TSTATE_FATAL:
			log_write(LOG_ERR,
			          "Failed to check whether %s is already installed\n",
			          name);
			return false;
	}

	if (TSTATE_OK != pkg_list_get(list, &entry, name))
		goto error;

	copy = strdup(name);
	if (NULL == copy)
		goto error;

	log_write(LOG_INFO, "Queueing %s for installation\n", copy);
	if (false == pkg_queue_push(queue, copy))
		goto free_copy;

	log_write(LOG_DEBUG, "Resolving the dependencies of %s\n", copy);
	params.queue = queue;
	params.list = list;
	params.root = (char *) root;
	if (TSTATE_OK == for_each_dep(entry.deps, dep_recurse, (void *) &params))
		return true;

free_copy:
	free(copy);

error:
	return false;
}

static tristate_t dep_cmp(const char *dep, void *arg)
{
	if (0 == strcmp(dep, (const char *) arg))
		return TSTATE_ERROR;

	return TSTATE_OK;
}

static tristate_t depends_on(const struct pkg_entry *entry, void *arg)
{
	tristate_t ret = TSTATE_OK;

	log_write(LOG_DEBUG,
	          "Checking whether %s depends on %s\n",
	          entry->name,
	          (const char *) arg);

	/* skip the package itself */
	if (0 == strcmp(entry->name, (const char *) arg))
		goto end;

	ret = for_each_dep(entry->deps, dep_cmp, arg);
	if (TSTATE_ERROR == ret) {
		log_write(LOG_DEBUG,
		          "%s depends on %s\n",
		          entry->name,
		          (const char *) arg);
	}

end:
	return ret;
}

tristate_t pkg_not_required(const char *name, const char *root)
{
	return pkg_entry_for_each(root, depends_on, (void *) name);
}

static tristate_t if_unneeded(const struct pkg_entry *entry, void *arg)
{
	struct unneeded_iter_params *params = (struct unneeded_iter_params *) arg;

	/* ignore all packages but those installed as dependencies */
	if (0 != strcmp(INST_REASON_DEP, entry->reason)) {
		log_write(LOG_DEBUG,
		          "%s cannot be removed; it is a %s package\n",
		          entry->name,
		          entry->reason);
		return TSTATE_OK;
	}

	switch (pkg_not_required(entry->name, params->root)) {
		case TSTATE_ERROR:
			log_write(LOG_DEBUG,
			          "Cannot remove %s; another package depends on it\n",
			          entry->name);
			return TSTATE_OK;

		case TSTATE_FATAL:
			log_write(
			     LOG_ERR,
			     "Cannot determine whether %s is required by another package\n",
			     entry->name);
			return TSTATE_FATAL;
	}

	log_write(LOG_INFO, "%s is no longer required; cleaning up\n", entry->name);
	if (false == params->cb(entry, params->arg))
		return TSTATE_ERROR;

	++params->removed;
	return TSTATE_OK;
}

tristate_t for_each_unneeded_pkg(const char *root,
                                 bool (*cb)(const struct pkg_entry *entry,
                                            void *arg),
                                 void *arg)
{
	struct unneeded_iter_params params;
	unsigned int total;
	tristate_t ret;

	log_write(LOG_DEBUG, "Looking for unneeded packages\n");

	params.cb = cb;
	params.arg = arg;
	params.root = (char *) root;
	total = 0;

	do {
		params.removed = 0;
		ret = pkg_entry_for_each(root, if_unneeded, (void *) &params);
		if (TSTATE_OK != ret)
			return ret;

		if (0 == params.removed)
			break;

		total += params.removed;
	} while (1);

	if (0 == total)
		log_write(LOG_DEBUG, "There are no unneeded packages\n");

	return TSTATE_OK;
}