#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pkg.h"
#include "tar.h"
#include "flist.h"
#include "log.h"
#include "pkg_ops.h"

static bool delete_file(const char *path, void *arg)
{
	log_write(LOG_DEBUG, "Removing %s\n", path);

	if (0 == unlink(path))
		return true;

	switch (errno) {
		case ENOENT:
			log_write(LOG_WARN, "%s is missing\n", path);
			return true;

		case EISDIR:
			if (0 == rmdir(path))
				return true;
			if (ENOTEMPTY == errno)
				return true;
	}

	log_write(LOG_ERR, "Failed to remove %s\n", path);
	return false;
}

static bool list_file(void *arg, const char *path)
{
	log_write(LOG_DEBUG, "Extracting %s\n", path);
	return flist_add((struct flist *) arg, path);
}

bool pkg_install(const char *path,
                 const struct pkg_entry *entry,
                 const char *root,
                 const bool check_sig)
{
	char dir[PATH_MAX];
	struct flist list;
	struct pkg pkg;
	bool ret = false;

	log_write(LOG_INFO, "Opening %s\n", entry->fname);
	if (false == pkg_open(&pkg, path))
		goto end;

	log_write(LOG_INFO, "Verifying %s\n", entry->fname);
	if (false == pkg_verify(&pkg, check_sig))
		goto close_pkg;

	(void) sprintf(dir, "%s"INST_DATA_DIR"/%s", root, entry->name);
	log_write(LOG_DEBUG, "Creating %s\n", dir);
	if (-1 == mkdir(dir, S_IRUSR | S_IWUSR)) {
		if (EEXIST != errno)
			goto close_pkg;
		log_write(LOG_WARN, "%s already exists\n", dir);
	}

	log_write(LOG_DEBUG, "Creating a file list for %s\n", entry->name);
	if (false == flist_open(&list, "w+", entry->name, root))
		goto close_pkg;

	log_write(LOG_DEBUG, "Extracting %s to %s\n", entry->fname, root);
	if (false == tar_extract(pkg.data,
	                         pkg.tar_size,
	                         root,
	                         list_file,
	                         (void *) &list))
		goto close_flist;

	/* upon failure to register the package, delete all the extracted files */
	log_write(LOG_INFO,
	          "Registering %s as a %s package\n",
	          entry->name,
	          entry->reason);
	if (false == pkg_entry_register(entry, root)) {
		(void) flist_for_each_reverse(&list, root, delete_file, NULL);
		if (true == flist_delete(&list))
			(void) rmdir(dir);
		goto close_flist;
	}

	log_write(LOG_INFO, "Successfully installed %s\n", entry->name);
	ret = true;

close_flist:
	flist_close(&list);

close_pkg:
	pkg_close(&pkg);

end:
	return ret;
}

bool pkg_remove(const char *name, const char *root)
{
	char dir[PATH_MAX];
	struct flist list;
	bool ret = false;

	log_write(LOG_DEBUG, "Opening the file list of %s\n", name);
	if (false == flist_open(&list, "r", name, root))
		goto end;

	log_write(LOG_DEBUG, "Removing files installed by %s\n", name);
	if (TSTATE_OK != flist_for_each_reverse(&list, root, delete_file, NULL))
		goto close_list;

	log_write(LOG_DEBUG, "Removing the file list of %s\n", name);
	if (false == flist_delete(&list))
		goto close_list;

	log_write(LOG_INFO, "Unregistering %s\n", name);
	if (false == pkg_entry_unregister(name, root))
		goto close_list;

	(void) sprintf(dir, "%s"INST_DATA_DIR"/%s", root, name);
	log_write(LOG_DEBUG, "Removing %s\n", dir);
	if (-1 == rmdir(dir))
		goto close_list;

	log_write(LOG_INFO, "Successfully removed %s\n", name);
	ret = true;

close_list:
	flist_close(&list);

end:
	return ret;
}
