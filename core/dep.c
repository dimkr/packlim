#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "dep.h"

bool for_each_dep(const char *deps,
                  bool (*cb)(const char *dep, void *arg),
                  void *arg)
{
	char *copy;
	char *dep;
	char *pos;
	bool ret = false;

	if (0 == strlen(deps)) {
		ret = true;
		goto end;
	}

	copy = strdup(deps);
	if (NULL == copy)
		goto end;

	dep = strtok_r(copy, " ", &pos);
	while (NULL != dep) {
		if (false == cb(dep, arg))
			goto free_copy;
		dep = strtok_r(NULL, " ", &pos);
	}

	ret = true;

free_copy:
	free(copy);

end:
	return ret;
}

static bool dep_recurse(const char *dep, void *arg)
{
	struct dep_queue_params *params = (struct dep_queue_params *) arg;
	return dep_queue(params->queue, params->list, dep, params->root);
}

bool dep_queue(struct pkg_queue *queue,
               struct pkg_list *list,
               const char *name,
               const char *root)
{
	struct pkg_entry entry;
	struct dep_queue_params params;
	char *copy;
	bool ret = false;

	if (true == pkgq_contains(queue, name)) {
		log_write(LOG_WARN, "%s is already queued for installation\n", name);
		ret = true;
		goto end;
	}

	if (true == pkgent_get(name, &entry, root)) {
		log_write(LOG_WARN, "%s is already installed\n", name);
		ret = true;
		goto end;
	}

	if (false == pkglist_get(list, &entry, name))
		goto end;

	copy = strdup(name);
	if (NULL == copy)
		goto end;

	log_write(LOG_INFO, "Queueing %s for installation\n", copy);
	if (false == pkgq_push(queue, copy)) {
		goto free_copy;
	}

	log_write(LOG_INFO, "Resolving the dependencies of %s\n", copy);
	params.queue = queue;
	params.list = list;
	params.root = (char *) root;
	if (false == for_each_dep(entry.deps, dep_recurse, (void *) &params))
		goto free_copy;

	ret = true;
	goto end;

free_copy:
	free(copy);

end:
	return ret;
}

static bool dep_cmp(const char *dep, void *arg)
{
	struct required_params *params = (struct required_params *) arg;

	if (0 == strcmp(dep, params->name)) {
		params->result = true;
		return false;
	}

	return true;
}

static bool depends_on(const struct pkg_entry *entry, void *arg)
{
	struct required_params *params = (struct required_params *) arg;

	/* skip the package itself */
	if (0 == strcmp(entry->name, params->name))
		return true;

	(void) for_each_dep(entry->deps, dep_cmp, arg);

	if (true == params->result) {
		log_write(LOG_DEBUG, "%s depends on %s\n", entry->name, params->name);
		return false;
	}

	return true;
}

bool pkg_required(const char *name, const char *root)
{
	struct required_params params;

	params.name = (char *) name;
	params.result = false;

	(void) pkgent_foreach(root, depends_on, (void *) &params);
	return params.result;
}

static bool if_unrequired(const struct pkg_entry *entry, void *arg)
{
	struct unrequired_iter_params *params = \
	                                      (struct unrequired_iter_params *) arg;

	/* ignore all packages but those installed as dependencies */
	if (0 != strcmp(INST_REASON_DEP, entry->reason))
		return true;

	if (true == pkg_required(entry->name, params->root)) {
		log_write(LOG_DEBUG,
		          "Cannot remove %s; another package depends on it\n",
		          entry->name);
		return true;
	}

	log_write(LOG_INFO, "%s is no longer required; cleaning up\n", entry->name);
	if (false == params->cb(entry, params->arg))
		return false;

	++params->removed;
	return true;
}

bool for_each_unneeded_pkg(const char *root,
                           bool (*cb)(const struct pkg_entry *entry,
                                      void *arg),
                           void *arg)
{
	struct unrequired_iter_params params;
	unsigned int total;

	log_write(LOG_INFO, "Looking for unneeded packages\n");

	params.cb = cb;
	params.arg = arg;
	params.root = (char *) root;
	total = 0;

	do {
		params.removed = 0;
		if (false == pkgent_foreach(root, if_unrequired, (void *) &params))
			return false;

		if (0 == params.removed)
			break;

		total += params.removed;
	} while (1);

	if (0 == total)
		log_write(LOG_INFO, "There are no unneeded packages\n");

	return true;
}