/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "crc32.h"
#include "err.h"
#include "iot.h"
#include "iotupd.h"
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

#define MY_HARDCODED_CHANNEL "wibble" /* FIXME */

lc_ctx_t *ctx = NULL;
lc_socket_t * sock = NULL;
lc_channel_t * chan = NULL;
char *map;
struct stat sb;

void sigint_handler (int signo);
void terminate();

void sigint_handler (int signo)
{
	terminate();
}

void terminate()
{
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	munmap(map, sb.st_size);
	_exit(0);
}

int main(int argc, char **argv)
{
	int fd;
	struct iot_frame_t f;
	uint32_t crc;
	byte fhash[HASHSIZE] = {};
	lc_message_t msg;

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

	/* calculate file hash */
	crc = crc_32((unsigned char *)map, sb.st_size);
	hash(fhash, map, sb.st_size);

	signal(SIGINT, sigint_handler);

	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	chan = lc_channel_new(ctx, MY_HARDCODED_CHANNEL);
	lc_channel_bind(sock, chan);

	memset(&f, 0, sizeof(iot_frame_t));

	while (1) {
		for (int i = 0; i <= sb.st_size; i += MTU_FIXED) {
			f.op = 0; /* TODO: data opcode */
			f.crc = crc;
			f.size = sb.st_size;
			f.off = i;

			if ((i + MTU_FIXED) > sb.st_size)
				f.len = sb.st_size - i;
			else
				f.len = MTU_FIXED;

			logmsg(LOG_DEBUG, "sending %i - %i", i, (int)(i+f.len));

			memcpy(f.hash, fhash, HASHSIZE);
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


