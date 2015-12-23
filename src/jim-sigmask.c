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

#include <signal.h>

#include <jim.h>
#include <jim-subcmd.h>

struct sigmask {
	sigset_t set;
	sigset_t oldset;
};

static const char * const signames[] = {"term", "int", NULL};
enum {
	SIGNAME_TERM,
	SIGNAME_INT
};

static int sigmask_cmd_pending(Jim_Interp *interp,
                               int argc,
                               Jim_Obj *const *argv)
{
	struct sigmask *mask = (struct sigmask *)Jim_CmdPrivData(interp);
	int out = 0, i, sig;

	if (sigpending(&mask->set) == -1) {
		return JIM_ERR;
	}

	for (i = 1; (!out) && (i < argc); ++i) {
		if (Jim_GetEnum(interp, argv[i], signames, &sig, "sig", JIM_ERRMSG | JIM_ENUM_ABBREV) != JIM_OK) {
			return JIM_ERR;
		}

		switch (sig) {
			case SIGNAME_TERM:
				out = sigismember(&mask->set, SIGTERM);
				break;

			case SIGNAME_INT:
				out = sigismember(&mask->set, SIGINT);
				break;
		}
	}

	Jim_SetResultBool(interp, out);
	return JIM_OK;
}

static const jim_subcmd_type sigmask_command_table[] = {
	{
		"pending",
		"sig ...",
		sigmask_cmd_pending,
		1,
		-1
	},
	{ NULL }
};

static int JimSigmaskHandlerCommand(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
    return Jim_CallSubCmd(interp, Jim_ParseSubCmd(interp, sigmask_command_table, argc, argv), argc, argv);
}

static void JimSigmaskDelProc(Jim_Interp *interp, void *privData)
{
	struct sigmask *mask = (struct sigmask *)privData;

	sigprocmask(SIG_SETMASK, &mask->oldset, NULL);
	Jim_Free(mask);
}

int Jim_SigmaskCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char buf[32];
	struct sigmask *mask;
	int i, sig, out;

	if (argc < 2) {
		Jim_WrongNumArgs(interp, 0, argv, "sig ...");
		return JIM_ERR;
	}

	mask = Jim_Alloc(sizeof(*mask));

	if (-1 == sigemptyset(&mask->set)) {
		Jim_Free(mask);
		return JIM_ERR;
	}

	for (i = 1; i < argc; ++i) {
		if (Jim_GetEnum(interp, argv[i], signames, &sig, "sig", JIM_ERRMSG | JIM_ENUM_ABBREV) != JIM_OK) {
			return JIM_ERR;
		}

		switch (sig) {
			case SIGNAME_TERM:
				out = sigaddset(&mask->set, SIGTERM);
				break;

			case SIGNAME_INT:
				out = sigaddset(&mask->set, SIGINT);
				break;
		}
		if (out == -1) {
			Jim_Free(mask);
			return JIM_ERR;
		}
	}

	if (sigprocmask(SIG_SETMASK, &mask->set, &mask->oldset) == -1) {
		Jim_Free(mask);
		return JIM_ERR;
	}

	sprintf(buf, "sigmask.%ld", Jim_GetId(interp));
	Jim_CreateCommand(interp,
	                  buf,
	                  JimSigmaskHandlerCommand,
	                  mask,
	                  JimSigmaskDelProc);
	Jim_SetResult(interp, Jim_MakeGlobalNamespaceName(interp, Jim_NewStringObj(interp, buf, -1)));

	return JIM_OK;
}
