/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2019-2021 Brett Sheffield <brett@gladserv.com> */

#include "err.h"
#include "iot.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <librecast.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PROGRAM_NAME "iotupd"

static lc_ctx_t *ctx = NULL;
static lc_socket_t *sock = NULL;
static lc_channel_t *chan = NULL;
static lc_message_t msg;
static int running = 1;
static int fd;
static char *map;
static struct stat sb;

void sigint_handler (int signo);
void terminate();

void sigint_handler (int signo)
{
	running = 0;
}

void terminate()
{
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	munmap(map, sb.st_size);
	close(fd);
	_exit(0);
}

int main(int argc, char **argv)
{
	struct iot_frame_t f;
	const int on = 1;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		logmsg(LOG_ERROR, "Unable to open file '%s'", argv[1]);
		return IOTD_ERROR_FILE_OPEN_FAIL;
	}

	if (fstat(fd, &sb) == -1) {
		logmsg(LOG_ERROR, "fstat() failed");
		return IOTD_ERROR_FILE_STAT_FAIL;
	}

	logmsg(LOG_DEBUG, "Mapping file '%s' with %lld bytes", argv[1], (long long)sb.st_size);

	map = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		logmsg(LOG_ERROR, "mmap() failed: %s", strerror(errno));
		return IOTD_ERROR_MMAP_FAIL;
	}

	signal(SIGINT, sigint_handler);

	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	lc_socket_setopt(sock, IPV6_MULTICAST_LOOP, &on, sizeof(on));
	chan = lc_channel_new(ctx, MY_HARDCODED_CHANNEL);
	lc_channel_bind(sock, chan);

	memset(&f, 0, sizeof(iot_frame_t));

	/* calculate file hash */
	hash(f.hash, map, sb.st_size);

	while (running) {
		for (int i = 0; i <= sb.st_size && running; i += MTU_FIXED) {
			f.op = 0; /* TODO: data opcode */
			f.size = sb.st_size;
			f.off = i;

			if ((i + MTU_FIXED) > sb.st_size)
				f.len = sb.st_size - i;
			else
				f.len = MTU_FIXED;

			logmsg(LOG_DEBUG, "sending %i - %i", i, (int)(i+f.len));

			memcpy(f.data, map + i, f.len);

			lc_msg_init_data(&msg, &f, sizeof(f), NULL, NULL);
			lc_msg_send(chan, &msg);
#ifdef PKT_DELAY
			usleep(PKT_DELAY);
#endif
		}
	}
	terminate();

	return 0;
}
