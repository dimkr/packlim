#include <jim.h>

enum {
	OPT_DEBUG,
	OPT_INFO,
	OPT_WARNING,
	OPT_ERROR,
	OPT_LEVEL
};

struct log {
	int min;
};

int Jim_PacklimLogCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
