#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "log.h"

static const char *g_levels[] = {
	"DEBUG",
	"INFO",
	"WARNING",
	"ERROR"
};

void log_write(const int level, const char *format, ...)
{
	char text_now[26];
	va_list args;
	FILE *stream;
	time_t now;

	(void) time(&now);
	if (NULL == ctime_r(&now, text_now))
		return;

	text_now[strlen(text_now) - 1] = '\0';

	(void) printf("[%s](%-7s): ", text_now, g_levels[level]);

	if (LOG_ERR == level)
		stream = stderr;
	else
		stream = stdout;

	va_start(args, format);
	(void) vfprintf(stream, format, args);
	va_end(args);
}
