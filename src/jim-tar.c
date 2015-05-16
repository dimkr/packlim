#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include <jim.h>
#include <archive.h>
#include <archive_entry.h>

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
                      Jim_Interp *interp,
                      int (*cb)(struct archive *,
                                struct archive *,
                                Jim_Interp *,
                                void *,
                                const char *),
                      void *arg)
{
	struct archive *in;
	struct archive *out;
	struct archive_entry *entry;
	const char *path;
	int ret = JIM_ERR;

	in = archive_read_new();
	if (NULL == in)
		goto end;

	out = archive_write_disk_new();
	if (NULL == out)
		goto close_in;

	archive_read_support_filter_lzip(in);
	archive_read_support_filter_xz(in);
	archive_read_support_filter_gzip(in);
	archive_read_support_format_tar(in);

	archive_write_disk_set_options(out,
	                               ARCHIVE_EXTRACT_OWNER |
	                               ARCHIVE_EXTRACT_PERM |
	                               ARCHIVE_EXTRACT_TIME |
	                               ARCHIVE_EXTRACT_UNLINK |
	                               ARCHIVE_EXTRACT_ACL |
	                               ARCHIVE_EXTRACT_FFLAGS |
	                               ARCHIVE_EXTRACT_XATTR);
	archive_write_disk_set_standard_lookup(out);

	if (0 != archive_read_open_memory(in, data, len))
		goto close_out;

	do {
		switch (archive_read_next_header(in, &entry)) {
			case ARCHIVE_OK:
				break;

			case ARCHIVE_EOF:
				ret = JIM_OK;

				/* fall through */

			default:
				goto close_out;
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

		if (JIM_OK != cb(in, out, interp, arg, &path[1]))
			break;
	} while (1);

close_out:
	(void) archive_write_close(out);
	archive_write_free(out);

close_in:
	archive_read_free(in);

end:
	return ret;
}

int Jim_TarListCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	Jim_Obj *files;
	unsigned char *data;
	int len;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "data");
		return JIM_ERR;
	}

	data = (unsigned char *) Jim_GetString(argv[1], &len);
	/* we assume sizeof(size_t) >= sizeof(int) */
	if ((0 >= len) || (UINT_MAX < len))
		return JIM_ERR;

	files = Jim_NewListObj(interp, NULL, 0);

	if (JIM_ERR == iter_files(data,
	                          (size_t) len,
	                          interp,
	                          list_file,
	                          files))
		return JIM_ERR;

	Jim_SetResult(interp, files);

	return JIM_OK;
}

int Jim_TarExtractCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	unsigned char *data;
	int len;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "data");
		return JIM_ERR;
	}

	data = (unsigned char *) Jim_GetString(argv[1], &len);
	if ((0 >= len) || (UINT_MAX < len))
		return JIM_ERR;

	if (JIM_ERR == iter_files(data,
	                          (size_t) len,
	                          interp,
	                          extract_file,
	                          NULL))
		return JIM_ERR;

	Jim_SetEmptyResult(interp);

	return JIM_OK;
}
