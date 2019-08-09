/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __IOTD_MISC_H__
#define __IOTD_MISC_H__ 1

#include <stdarg.h>
#include <sys/types.h>

/* return size of buffer to allocate for vsnprintf() */
int _vscprintf (const char * format, va_list argp);

#endif /* __IOTD_MISC_H__ */
