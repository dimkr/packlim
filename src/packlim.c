#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <jim.h>
#include <curl/curl.h>

extern int Jim_LockfLockCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_LockfTestCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_Ed25519VerifyCmd(Jim_Interp *interp,
                                int argc,
                                Jim_Obj *const *argv);
extern int Jim_TarListCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_TarExtractCmd(Jim_Interp *interp,
                             int argc,
                             Jim_Obj *const *argv);
extern int Jim_CurlCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);

static const char packlim_tcl[] = {
#include "packlim.inc"
};

static int Jim_packlimInit(Jim_Interp *jim)
{
	Jim_CreateCommand(jim,
	                  "lockf.lock",
	                  Jim_LockfLockCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "lockf.locked",
	                  Jim_LockfTestCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "ed25519.verify",
	                  Jim_Ed25519VerifyCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "tar.list",
	                  Jim_TarListCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "tar.extract",
	                  Jim_TarExtractCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "curl",
	                  Jim_CurlCmd,
	                  NULL,
	                  NULL);

	if (JIM_OK != Jim_PackageProvide(jim, "packlim", VERSION, JIM_ERRMSG))
		return JIM_ERR;
	if (JIM_OK != Jim_EvalSource(jim, "packlim.inc", 1, packlim_tcl))
		return JIM_ERR;

	return JIM_OK;
}

int main(int argc, char *argv[])
{
	const char *prefix;
	Jim_Interp *jim;
	Jim_Obj *argv_obj;
	int i;
	int ret = EXIT_FAILURE;

	if (0 != geteuid()) {
		(void) write(STDERR_FILENO, "Error: must run as root.\n", 25);
		goto end;
	}

	jim = Jim_CreateInterp();
	if (NULL == jim)
		goto end;

	Jim_RegisterCoreCommands(jim);
	Jim_InitStaticExtensions(jim);

	argv_obj = Jim_NewListObj(jim, NULL, 0);

	for (i = 0; argc > i; ++i) {
		Jim_ListAppendElement(jim,
		                      argv_obj,
		                      Jim_NewStringObj(jim, argv[i], -1));
	}

	Jim_SetVariableStr(jim, "argv", argv_obj);
	Jim_SetVariableStr(jim, "argc", Jim_NewIntObj(jim, argc));

	if (JIM_OK != Jim_packlimInit(jim))
		goto free_jim;

	if (0 != curl_global_init(CURL_GLOBAL_NOTHING))
		goto free_jim;

	prefix = getenv("PREFIX");
	if (NULL != prefix) {
		if (-1 == chroot(prefix))
			goto free_jim;
		if (-1 == chdir("/"))
			goto free_jim;
	}

	Jim_Eval(jim, "main");

	ret = Jim_GetExitCode(jim);

	curl_global_cleanup();

free_jim:
	Jim_FreeInterp(jim);

end:
	return ret;
}
