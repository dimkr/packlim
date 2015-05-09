#include <string.h>
#include <time.h>
#include <stdio.h>
#include <syslog.h>

#include "log.h"

static int log_write(FILE *stream,
                     const struct log *log,
                     const int option,
                     const int argc,
                     Jim_Obj *const *argv)
{
	char text_now[26];
	time_t now;
	const char *msg;
	int prio;
	int len;

	if (log->min > option)
		return JIM_OK;

	msg = Jim_GetString(argv[2], &len);
	if (0 == len)
		return JIM_ERR;

	(void) time(&now);
	if (NULL == ctime_r(&now, text_now))
		return JIM_ERR;
	text_now[strlen(text_now) - 1] = '\0';

	if (0 > fprintf(stream, "[%s]<%d>: %s\n", text_now, option, msg))
		return JIM_ERR;

	switch (option) {
		case OPT_DEBUG:
			prio = LOG_DEBUG;
			break;

		case OPT_INFO:
			prio = LOG_INFO;
			break;

		case OPT_WARNING:
			prio = LOG_WARNING;
			break;

		default:
			prio = LOG_ERR;
			break;
	}

	syslog(prio, "%s", msg);

	return JIM_OK;
}

static int JimLogHandlerCommand(Jim_Interp *interp,
                                int argc,
                                Jim_Obj *const *argv)
{
	static const char *const options[] = {
		"debug", "info", "warning", "error", "level", NULL
	};
	static const char *const levels[] = {
		"debug", "info", "warning", "error", NULL
	};
	struct log *log = (struct log *) Jim_CmdPrivData(interp);
	int option;
	int level;

	if (3 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "method ?args ...?");
		return JIM_ERR;
	}

	if (JIM_OK != Jim_GetEnum(interp,
	                          argv[1],
	                          options,
	                          &option,
	                          "log method",
	                          JIM_ERRMSG))
		return JIM_ERR;

	switch (option) {
		case OPT_DEBUG:
		case OPT_INFO:
		case OPT_WARNING:
			return log_write(stdout, log, option, argc, argv);

		case OPT_ERROR:
			return log_write(stderr, log, option, argc, argv);

		case OPT_LEVEL:
			if (JIM_OK != Jim_GetEnum(interp,
			                          argv[2],
			                          levels,
			                          &level,
			                          "log verbosity level",
			                          JIM_ERRMSG))
				return JIM_ERR;

			log->min = level;
			return JIM_OK;
	}

	return JIM_OK;
}

static void JimLogDelProc(Jim_Interp *interp, void *privData)
{
	struct log *log = (struct log *) privData;

	closelog();
	Jim_Free(log);
}

int Jim_PacklimLogCmd(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	char buf[32];
	struct log *log;
	const char *ident;
	int len;

	if (2 != argc) {
		Jim_WrongNumArgs(interp, 1, argv, "ident");
		return JIM_ERR;
	}

	log = Jim_Alloc(sizeof(*log));
	if (NULL == log)
		return JIM_ERR;

	ident = Jim_GetString(argv[1], &len);
	if (0 == len)
		return JIM_ERR;

	openlog(ident, LOG_NDELAY, LOG_USER);

	log->min = OPT_INFO;

	(void) sprintf(buf, "log.handle%ld", Jim_GetId(interp));
	Jim_CreateCommand(interp,
	                  buf,
	                  JimLogHandlerCommand,
	                  log,
	                  JimLogDelProc);
	Jim_SetResult(interp,
	              Jim_MakeGlobalNamespaceName(interp,
	                                          Jim_NewStringObj(interp,
	                                                           buf,
	                                                           -1)));

	return JIM_OK;
}
