#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <limits.h>

#include <archive_entry.h>
#include <ed25519.h>

#include "package.h"

static int list_file(struct archive *in,
                     struct archive *out,
                     Jim_Interp *interp,
                     void *arg,
                     const char *path)
{
	Jim_ListAppendElement(interp,
	                      (Jim_Obj *) arg,
	                      Jim_NewStringObj(interp, path, -1));

	return JIM_OK;
}

static int extract_file(struct archive *in,
                        struct archive *out,
                        Jim_Interp *interp,
                        void *arg,
                        const char *path)
{
	__LA_INT64_T off;
	size_t size;
	const void *blk;

	do {
		switch (archive_read_data_block(in, &blk, &size, &off)) {
			case ARCHIVE_OK:
				if (ARCHIVE_OK != archive_write_data_block(out,
				                                           blk,
				                                           size,
				                                           off))
					return JIM_ERR;
				break;

			case ARCHIVE_EOF:
				return JIM_OK;

			default:
				return JIM_ERR;
		}
	} while (1);

	return JIM_ERR;
}

static int iter_files(unsigned char *data,
                      const size_t len,
                      const char *dest,
                      Jim_Interp *interp,
                      int (*cb)(struct archive *,
                                struct archive *,
                                Jim_Interp *,
                                void *,
                                const char *),
                      void *arg)
{
	char cwd[PATH_MAX];
	struct archive *in;
	struct archive *out = NULL;
	struct archive_entry *entry;
	const char *path;
	int ret = JIM_ERR;

	in = archive_read_new();
	if (NULL == in)
		goto end;

	if (NULL != dest) {
		if (NULL == getcwd(cwd, sizeof(cwd)))
			goto end;

		out = archive_write_disk_new();
		if (NULL == out)
			goto close_in;
	}

	archive_read_support_filter_lzip(in);
	archive_read_support_filter_xz(in);
	archive_read_support_filter_gzip(in);
	archive_read_support_format_tar(in);

	if (NULL != out) {
		archive_write_disk_set_options(out, EXTRACTION_OPTIONS);
		archive_write_disk_set_standard_lookup(out);
	}

	if (0 != archive_read_open_memory(in, data, len))
		goto close_out;

	if (NULL != dest) {
		if (-1 == chdir(dest))
			goto close_out;
	}

	do {
		switch (archive_read_next_header(in, &entry)) {
			case ARCHIVE_OK:
				break;

			case ARCHIVE_EOF:
				ret = JIM_OK;

				/* fall through */

			default:
				goto restore_wd;
		}

		path = archive_entry_pathname(entry);
		if (NULL == path)
			break;

		/* make sure all paths begin with "./" */
		if (0 != strncmp("./", path, 2))
			break;

		/* ignore the root directory */
		if (0 == strcmp("./", path))
			continue;

		if (NULL != out) {
			if (ARCHIVE_OK != archive_write_header(out, entry))
				break;
		}

		if (JIM_OK != cb(in, out, interp, arg, path))
			break;
	} while (1);

restore_wd:
	if (NULL != dest)
		(void) chdir(cwd);

close_out:
	if (NULL != out) {
		(void) archive_write_close(out);
		archive_write_free(out);
	}

close_in:
	archive_read_free(in);

end:
	return ret;
}

static int verify_pkg(Jim_Interp *interp,
                      const struct pkg *pkg,
                      const unsigned char *key)
{
	if (0 == ed25519_verify(pkg->hdr->sig, pkg->data, pkg->tar_len, key))
		return JIM_ERR;

	return JIM_OK;
}

static int JimPackageHandlerCommand(Jim_Interp *interp,
                                    int argc,
                                    Jim_Obj *const *argv)
{
	static const char *const options[] = {
		"extract", "list", "verify", NULL
	};
	const char *dest;
	const unsigned char *key;
	struct pkg *pkg = (struct pkg *) Jim_CmdPrivData(interp);
	Jim_Obj *list;
	int option;
	int len;
	enum { OPT_EXTRACT, OPT_LIST, OPT_VERIFY };

	if (2 > argc) {
		Jim_WrongNumArgs(interp, 1, argv, "method ?args ...?");
		return JIM_ERR;
	}

	if (JIM_OK != Jim_GetEnum(interp,
	                          argv[1],
	                          options,
	                          &option,
	                          "package method",
	                          JIM_ERRMSG))
		return JIM_ERR;

	switch (option) {
		case OPT_EXTRACT:
			if (3 != argc)
				return JIM_ERR;

			dest = Jim_GetString(argv[2], &len);
			if (0 == len)
				return JIM_ERR;

			return iter_files(pkg->data,
			                  pkg->tar_len,
			                  dest,
			                  interp,
			                  extract_file,
			                  NULL);

		case OPT_LIST:
			if (2 != argc)
				return JIM_ERR;

			list = Jim_NewListObj(interp, NULL, 0);

			if (JIM_ERR == iter_files(pkg->data,
			                          pkg->tar_len,
			                          NULL,
			                          interp,
			                          list_file,
			                          list))
				return JIM_ERR;

			Jim_SetResult(interp, list);

			return JIM_OK;

		case OPT_VERIFY:
			if (3 != argc)
				return JIM_ERR;

			/* the key must be 64 bytes long, according to the ed25519_verify()
			 * documentation */
			key = (const unsigned char *) Jim_GetString(argv[2], &len);
			if (64 != len)
				return JIM_ERR;

			return verify_pkg(interp, pkg, key);
	}

	return JIM_OK;
}

static void JimPackageDelProc(Jim_Interp *interp, void *privData)
{
	struct pkg *pkg = (struct pkg *) privData;

	(void) munmap(pkg->data, pkg->len);
	(void) close(pkg->fd);
	Jim_Free(pkg);
}

int Jim_PacklimPackageCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	struct stat stbuf;
	const char *path;
	struct pkg *pkg;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "path");
		return JIM_ERR;
	}

	pkg = Jim_Alloc(sizeof(*pkg));
	if (NULL == pkg)
		return JIM_ERR;

	path = Jim_String(argv[1]);

	pkg->fd = open(path, O_RDONLY);
	if (-1 == pkg->fd)
		goto free_pkg;

	if (-1 == fstat(pkg->fd, &stbuf))
		goto free_pkg;

	pkg->len = (size_t) stbuf.st_size;
	if (sizeof(struct pkg_hdr) >= pkg->len)
		goto close_pkg;

	pkg->data = mmap(NULL, pkg->len, PROT_READ, MAP_PRIVATE, pkg->fd, 0);
	if (MAP_FAILED == pkg->data)
		goto close_pkg;

	pkg->tar_len = pkg->len - sizeof(struct pkg_hdr);
	pkg->hdr = (const struct pkg_hdr *) ((unsigned char *) pkg->data +
	                                      pkg->tar_len);

	if (PKG_MAGIC != pkg->hdr->magic)
		goto unmap;

	Jim_CreateCommand(interp,
	                  path,
	                  JimPackageHandlerCommand,
	                  pkg,
	                  JimPackageDelProc);
	Jim_SetResult(interp, Jim_MakeGlobalNamespaceName(interp, argv[1]));

	return JIM_OK;

unmap:
	(void) munmap(pkg->data, pkg->len);

close_pkg:
	(void) close(pkg->fd);

free_pkg:
	Jim_Free(pkg);

	return JIM_ERR;
}
