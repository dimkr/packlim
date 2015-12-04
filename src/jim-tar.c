/*
 * this file is part of packlim.
 *
 * Copyright (c) 2015 Dima Krasner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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
					goto error;
				break;

			case ARCHIVE_EOF:
				return JIM_OK;

			default:
				goto error;
		}
	} while (1);

error:
	Jim_SetResultFormatted(interp, "failed to extract %s", path);
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
	struct archive *in, *out;
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

	if (0 != archive_read_open_memory(in, data, len)) {
		Jim_SetResultFormatted(interp, "failed to read an archive: %s", archive_error_string(in), -1);
		goto close_out;
	}

	do {
		switch (archive_read_next_header(in, &entry)) {
			case ARCHIVE_OK:
				break;

			case ARCHIVE_EOF:
				ret = JIM_OK;
				goto close_out;

			default:
				Jim_SetResultString(interp, "failed to read a file header", -1);
				goto close_out;
		}

		path = archive_entry_pathname(entry);
		if (NULL == path)
			break;

		/* make sure all paths begin with "./" */
		if (0 != strncmp("./", path, 2)) {
			Jim_SetResultFormatted(interp,
			                       "read an invalid file path: %s",
			                       path);
			break;
		}

		/* ignore the root directory */
		if (0 == strcmp("./", path))
			continue;

		if (ARCHIVE_OK != archive_write_header(out, entry)) {
			Jim_SetResultString(interp, "failed to write a file header", -1);
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

static int check_len(Jim_Interp *interp,
                     Jim_Obj *obj,
                     unsigned char **data,
                     int *len)
{
	*data = (unsigned char *) Jim_GetString(obj, len);

	if (0 == len) {
		Jim_SetResultString(interp, "the archive is an empty file", -1);
		return JIM_ERR;
	}

	return JIM_OK;
}

int Jim_TarListCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	Jim_Obj *files;
	unsigned char *data;
	int len, ret = JIM_ERR;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "data");
		goto out;
	}

	Jim_SetEmptyResult(interp);

	if (JIM_ERR == check_len(interp, argv[1], &data, &len))
		goto out;

	files = Jim_NewListObj(interp, NULL, 0);
	Jim_IncrRefCount(files);

	ret = iter_files(data, (size_t) len, interp, list_file, files);
	if (JIM_OK == ret)
		Jim_SetResult(interp, files);

	Jim_DecrRefCount(interp, files);

out:
	return ret;
}

int Jim_TarExtractCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	unsigned char *data;
	int len;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "data");
		return JIM_ERR;
	}

	Jim_SetEmptyResult(interp);

	if (JIM_ERR == check_len(interp, argv[1], &data, &len))
		return JIM_ERR;

	return iter_files(data, (size_t) len, interp, extract_file, NULL);
}
