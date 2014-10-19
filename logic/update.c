#include "../core/repo.h"
#include "../core/pkg_list.h"
#include "../core/log.h"

bool packlad_update(const char *root, const char *url)
{
	char path[PATH_MAX];
	struct repo repo;
	bool ret = false;

	log_write(LOG_INFO, "Updating the package list\n");

	(void) sprintf(path, "%s"PKG_LIST_PATH, root);

	if (false == repo_open(&repo, url))
		goto end;

	ret = repo_fetch(&repo, PKG_LIST_FILE_NAME, path);
	if (true == ret)
		log_write(LOG_INFO, "Successfully updated the package list\n");

	repo_close(&repo);

end:
	return ret;
}
