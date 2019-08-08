/* SPDX-License-Identifier: GPL-2.0-only */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "err.h"
#include "log.h"

int err_log(int level, int e)
{
	LOG(level, "%s", err_msg(e));
	return e;
}

char *err_msg(int e)
{
	switch (e) {
		IOTD_ERROR_CODES(IOTD_ERROR_MSG)
	}
	return "Unknown error";
}

void err_print(int e, int errsv, char *errstr)
{
	char buf[LINE_MAX];
	if (errsv != 0) {
		strerror_r(errsv, buf, sizeof(buf));
		LOG(LOG_SEVERE, "%s: %s", errstr, buf);
	}
	else if (e != 0) {
		LOG(LOG_SEVERE, "%s: %s", errstr, err_msg(e));
	}
}
