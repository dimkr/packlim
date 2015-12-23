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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <jim.h>
#include <curl/curl.h>

extern int Jim_LockfLockCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_LockfTestCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_SigmaskCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_Ed25519VerifyCmd(Jim_Interp *interp,
                                int argc,
                                Jim_Obj *const *argv);
extern int Jim_TarListCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
extern int Jim_TarExtractCmd(Jim_Interp *interp,
                             int argc,
                             Jim_Obj *const *argv);
extern int Jim_CurlCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv);

extern int Jim_packlimInit(Jim_Interp *interp);

static const char main_tcl[] = {
#include "main.inc"
};

int main(int argc, char *argv[])
{
	const char *pfix;
	Jim_Interp *jim;
	Jim_Obj *argv_obj;
	int i;
	int ret = EXIT_FAILURE;

	if (geteuid() != 0) {
		write(STDERR_FILENO, "Error: must run as root.\n", 25);
		return EXIT_FAILURE;
	}

	pfix = getenv("PREFIX");
	if (NULL == pfix) {
		pfix = "/";
	}
	if (-1 == chdir(pfix)) {
		return EXIT_FAILURE;
	}

	if (0 != curl_global_init(CURL_GLOBAL_NOTHING)) {
		return EXIT_FAILURE;
	}

	jim = Jim_CreateInterp();
	if (NULL == jim) {
		curl_global_cleanup();
		return EXIT_FAILURE;
	}

	Jim_RegisterCoreCommands(jim);
	Jim_InitStaticExtensions(jim);

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
	                  "sigmask",
	                  Jim_SigmaskCmd,
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

	if (JIM_OK != Jim_packlimInit(jim)) {
		Jim_FreeInterp(jim);
		curl_global_cleanup();
		return EXIT_FAILURE;
	}

	argv_obj = Jim_NewListObj(jim, NULL, 0);

	for (i = 0; argc > i; ++i) {
		Jim_ListAppendElement(jim,
		                      argv_obj,
		                      Jim_NewStringObj(jim, argv[i], -1));
	}

	Jim_SetVariableStr(jim, "argv", argv_obj);
	Jim_SetVariableStr(jim, "argc", Jim_NewIntObj(jim, argc));

	Jim_Eval(jim, main_tcl);
	ret = Jim_GetExitCode(jim);

	Jim_FreeInterp(jim);
	curl_global_cleanup();

	return ret;
}
