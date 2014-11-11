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

static int g_min_level = LOG_INFO;

void log_write(const int level, const char *format, ...)
{
	char text_now[26];
	va_list args;
	FILE *stream;
	time_t now;

	if (level < g_min_level)
		return;

	(void) time(&now);
	if (NULL == ctime_r(&now, text_now))
		return;
	text_now[strlen(text_now) - 1] = '\0';

	if (LOG_ERR == level)
		stream = stderr;
	else
		stream = stdout;

	if (0 > fprintf(stream, "[%s](%-7s): ", text_now, g_levels[level]))
		return;

	va_start(args, format);
	(void) vfprintf(stream, format, args);
	va_end(args);
}

int log_get_min_level(void)
{
	return g_min_level;
}

void log_set_min_level(const int level)
{
	g_min_level = level;
}
