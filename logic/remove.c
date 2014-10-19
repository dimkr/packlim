#include <string.h>

#include "../core/pkg_entry.h"
#include "../core/dep.h"
#include "../core/pkg_ops.h"
#include "../core/log.h"

#include "cleanup.h"
#include "remove.h"

bool packlad_remove(const char *name, const char *root)
{
	struct pkg_entry entry;
	bool ret = false;

	if (false == pkgent_get(name, &entry, root)) {
		log_write(LOG_ERR, "%s is not installed\n", name);
		goto end;
	}

	if (0 != strcmp(INST_REASON_USER, entry.reason)) {
		log_write(LOG_ERR,
		          "%s cannot be removed; it is a %s package\n",
		          name,
		          entry.reason);
		goto end;
	}

	if (true == pkg_required(name, root))
		goto end;

	if (false == pkg_remove(name, root))
		goto end;

	(void) packlad_cleanup(root);

	ret = true;

end:
	return ret;
}
