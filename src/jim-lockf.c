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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <jim.h>

struct lock {
	char *path;
	int fd;
};

static void JimLockDelProc(Jim_Interp *interp, void *privData)
{
	struct lock *lock = (struct lock *)privData;

	lockf(lock->fd, F_ULOCK, 0);
	close(lock->fd);
	unlink(lock->path);

	Jim_Free(lock);
}

static int JimLockHandlerCommand(Jim_Interp *interp,
                                 int argc,
                                 Jim_Obj *const *argv)
{
	Jim_SetEmptyResult(interp);
	return JIM_ERR;
}

int Jim_LockfLockCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	struct lock *lock;
	const char *path;
	int len;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 0, argv, "path");
		return JIM_ERR;
	}

	path = Jim_GetString(argv[1], &len);
	if (!len) {
		Jim_SetResultString(interp,
		                    "the lock file path cannot be an empty string",
		                    -1);
		return JIM_ERR;
	}

	lock = Jim_Alloc(sizeof(*lock));

	lock->path = Jim_StrDupLen(path, len);
	if (!lock->path) {
		goto free_lock;
	}

	lock->fd = open(path,
	                O_WRONLY | O_CREAT,
	                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (lock->fd == -1) {
		Jim_SetResultFormatted(interp, "failed to open %s", path);
		goto free_path;
	}

	if (lockf(lock->fd, F_LOCK, 0) == -1) {
		Jim_SetResultFormatted(interp, "failed to lock %s", path);
		goto close_fd;
	}

	Jim_CreateCommand(interp,
	                  path,
	                  JimLockHandlerCommand,
	                  lock,
	                  JimLockDelProc);
	Jim_SetResult(interp, Jim_MakeGlobalNamespaceName(interp, argv[1]));

	return JIM_OK;

close_fd:
	close(lock->fd);

free_path:
	Jim_Free(lock->path);

free_lock:
	Jim_Free(lock);

	return JIM_ERR;
}

int Jim_LockfTestCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	const char *path;
	int len, fd, ret = JIM_ERR;

	if (argc != 2) {
		Jim_WrongNumArgs(interp, 0, argv, "path");
		return JIM_ERR;
	}

	path = Jim_GetString(argv[1], &len);
	if (!len) {
		Jim_SetResultString(interp,
		                    "the lock file path cannot be an empty string",
		                    -1);
		return JIM_ERR;
	}

	fd = open(path, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		if (errno != ENOENT) {
			Jim_SetResultFormatted(interp, "failed to open %s", path);
			return JIM_ERR;
		}

		Jim_SetResultBool(interp, 0);
		return JIM_OK;
	}

	if (lockf(fd, F_TEST, 0) == 0) {
		Jim_SetResultBool(interp, 0);
		ret = JIM_OK;
		goto close_fd;
	}

	if ((errno != EAGAIN) && (errno !=  EACCES)) {
		Jim_SetResultFormatted(interp,
		                       "failed to check whether %s is locked",
		                       path);
		goto close_fd;
	}

	Jim_SetResultBool(interp, 1);
	ret = JIM_OK;

close_fd:
	close(fd);

	return ret;
}
