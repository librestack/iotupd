/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2019 Brett Sheffield <brett@gladserv.com> */

#include <stdio.h>
#include <stdlib.h>
#include "misc.h"

int _vscprintf (const char * format, va_list argp)
{
	int r;
	va_list argc;
	va_copy(argc, argp);
	r = vsnprintf(NULL, 0, format, argc);
	va_end(argc);
	return r;
}
