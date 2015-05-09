#include <stdlib.h>

#include <jim.h>
#include <curl/curl.h>

#include "wait.h"
#include "log.h"
#include "repository.h"
#include "package.h"

static const char packlim_tcl[] = {
#include "packlim.inc"
};

static int Jim_packlimInit(Jim_Interp *jim)
{
	if (JIM_OK != Jim_PackageProvide(jim, "packlim", VERSION, JIM_ERRMSG))
		return JIM_ERR;

	if (JIM_OK != Jim_EvalSource(jim, "packlim.inc", 1, packlim_tcl))
		return JIM_ERR;

	Jim_CreateCommand(jim,
	                  "packlim.wait",
	                  Jim_PacklimWaitCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "packlim.log",
	                  Jim_PacklimLogCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "packlim.repository",
	                  Jim_PacklimRepositoryCmd,
	                  NULL,
	                  NULL);
	Jim_CreateCommand(jim,
	                  "packlim.package",
	                  Jim_PacklimPackageCmd,
	                  NULL,
	                  NULL);

	return JIM_OK;
}

int main(int argc, char *argv[])
{
	Jim_Interp *jim;
	Jim_Obj *argv_obj;
	int i;
	int ret = EXIT_FAILURE;

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

	Jim_Eval(jim, "packlim_main");

	ret = Jim_GetExitCode(jim);

	curl_global_cleanup();

free_jim:
	Jim_FreeInterp(jim);

end:
	return ret;
}
