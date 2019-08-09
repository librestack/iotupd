/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "misc.h"

void logmsg(unsigned int level, const char *fmt, ...)
{
	va_list argp;
	char *b;

	va_start(argp, fmt);
	b = malloc(_vscprintf(fmt, argp) + 1);
	assert(b != NULL);
	vsprintf(b, fmt, argp);
	va_end(argp);
	fprintf(stderr, "%s\n", b);
	free(b);
}

