/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2019-2022 Brett Sheffield <brett@librecast.net> */

#ifndef __IOTD_LOG
#define __IOTD_LOG 1

#define LOG_LEVELS(X) \
	X(0,    LOG_NONE,       "none")                                 \
	X(1,    LOG_SEVERE,     "severe")                               \
	X(2,    LOG_ERROR,      "error")                                \
	X(4,    LOG_WARNING,    "warning")                              \
	X(8,    LOG_INFO,       "info")                                 \
	X(16,   LOG_TRACE,      "trace")                                \
	X(32,   LOG_FULLTRACE,  "fulltrace")                            \
	X(64,   LOG_DEBUG,      "debug")
#undef X

#define LOG_ENUM(id, name, desc) name = id,
enum {
	LOG_LEVELS(LOG_ENUM)
};

#define loglevel 15

#define LOG(lvl, fmt, ...) if ((lvl & loglevel) == lvl) logmsg(lvl, fmt __VA_OPT__(,) __VA_ARGS__)
#define DEBUG(fmt, ...) LOG(LOG_DEBUG, fmt ,##__VA_ARGS__)
#define ERROR(fmt, ...) LOG(LOG_ERROR, fmt ,##__VA_ARGS__)

void logmsg(unsigned int level, const char *fmt, ...);

#endif /* __IOTD_LOG */
