/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2019-2022 Brett Sheffield <brett@librecast.net> */

#ifndef __IOTD_MISC_H__
#define __IOTD_MISC_H__ 1

#include <stdarg.h>
#include <sys/types.h>

/* return size of buffer to allocate for vsnprintf() */
int _vscprintf (const char * format, va_list argp);

#endif /* __IOTD_MISC_H__ */
