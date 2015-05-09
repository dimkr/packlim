#include <jim.h>

struct lock {
	char *path;
	int fd;
};

int Jim_PacklimWaitCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
