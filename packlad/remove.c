#include <string.h>

#include <libpkg/pkgent.h>
#include <libpkg/dep.h>
#include <libpkg/ops.h>
#include <libpkg/log.h>

#include "cleanup.h"
#include "remove.h"

bool packlad_remove(const char *name, const char *root)
{
	struct pkg_entry entry;
	bool ret = false;

	if (false == pkgent_get(name, &entry, root))
		goto end;

	if (0 != strcmp(INST_REASON_USER, entry.reason)) {
		log_write(LOG_ERR,
		          "%s cannot be removed; it is a %s package\n",
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
