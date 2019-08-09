/* SPDX-License-Identifier: GPL-2.0-only */

#include "err.h"
#include "iot.h"
#include "iotupd.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <librecast.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MY_HARDCODED_CHANNEL "wibble" /* FIXME */

/* IPv6 path discovery isn't much use for multicast and
 * we don't want to receive a bunch of Packet Too Big messages
 * so we'll use a fixed MTU of 1280 - headers + extensions => ~1200 */
#define MTU_FIXED 1200

lc_ctx_t *ctx = NULL;

int main(int argc, char **argv)
{
	int i, fd;
	struct stat sb;
	struct iot_frame_t f;
	char *map;
	lc_socket_t * sock = NULL;
	lc_channel_t * chan = NULL;
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

	/* TODO: signal handlers */

	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	chan = lc_channel_new(ctx, MY_HARDCODED_CHANNEL);
	lc_channel_bind(sock, chan);

	memset(&f, 0, sizeof(iot_frame_t));

	for (i = 0; i <= sb.st_size; i += MTU_FIXED) {
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

		/* TODO: occasionally send a checksum packet */

	} /* TODO: loop forever */

	logmsg(LOG_DEBUG, "%lld bytes sent", (long long)sb.st_size);

	/* clean up */
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	munmap(map, sb.st_size);

	return 0;
}
