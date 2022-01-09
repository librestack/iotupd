#include "iot.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

void *progress_update(void *arg)
{
	enum { buflen = 80 };
	struct winsize ws;
	size_t sz;
	char buf[buflen];
	struct timespec ts = { .tv_sec = 1 };
	if (!isatty(STDOUT_FILENO)) return arg;
	sem_init(&semprogress, 0, 0);
	do {
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) return arg;
		if (byt_in) {
			float pc = (float)byt_in / (float)maplen * 100;
			sz = snprintf(buf, buflen, "\rbytes received %zu / %zu (%0.2f %%)", byt_in, maplen, pc);
		}
		else if (byt_out)
			sz = snprintf(buf, buflen, "\rbytes sent %zu", byt_out);
		if (byt_in || byt_out) write(STDOUT_FILENO, buf, sz);
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec++;
	}
	while (sem_timedwait(&semprogress, &ts) == -1 && errno == ETIMEDOUT);
	sem_destroy(&semprogress);
	return arg;
}
