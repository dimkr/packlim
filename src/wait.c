#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "wait.h"

static void JimWaitDelProc(Jim_Interp *interp, void *privData)
{
	struct lock *lock = (struct lock *) privData;

	(void) lockf(lock->fd, F_ULOCK, 0);
	(void) close(lock->fd);
	(void) unlink(lock->path);

	Jim_Free(lock);
}

static int JimWaitHandlerCommand(Jim_Interp *interp,
                                 int argc,
                                 Jim_Obj *const *argv)
{
	return JIM_ERR;
}

int Jim_PacklimWaitCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	struct lock *lock;
	const char *path;
	int len;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "path");
		goto end;
	}

	path = Jim_GetString(argv[1], &len);
	if (0 == len)
		goto end;

	lock = Jim_Alloc(sizeof(*lock));
	if (NULL == lock)
		goto end;

	lock->path = Jim_StrDupLen(path, len);
	if (NULL == lock->path)
		goto free_lock;

	lock->fd = open(path,
	                O_WRONLY | O_CREAT,
	                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (-1 == lock->fd)
		goto free_path;

	if (-1 == lockf(lock->fd, F_TEST, 0)) {
		switch (errno) {
			case EAGAIN:
			case EACCES:
				break;

			default:
				goto close_fd;
		}
	}

	if (-1 == lockf(lock->fd, F_LOCK, 0))
		goto close_fd;

	Jim_CreateCommand(interp,
	                  path,
	                  JimWaitHandlerCommand,
	                  lock,
	                  JimWaitDelProc);
	Jim_SetResult(interp, Jim_MakeGlobalNamespaceName(interp, argv[1]));

	return JIM_OK;

close_fd:
	(void) close(lock->fd);

free_path:
	Jim_Free(lock->path);

free_lock:
	Jim_Free(lock);

end:
	return JIM_ERR;
}
