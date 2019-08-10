/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "err.h"
#include "iot.h"
#include "iotupc.h"
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
#include <wolfssl/ssl.h>

#define MY_HARDCODED_CHANNEL "wibble" /* FIXME */

int main(int argc, char **argv)
{
	int fd = 0;
	int ret = 0;
	struct stat sb;
	struct iot_frame_t *f;
	char *map = NULL;
	byte fhash[HASHSIZE];
	u_int64_t bwrit = 0;
	lc_ctx_t *ctx = NULL;
	lc_socket_t * sock = NULL;
	lc_channel_t * chan = NULL;
	lc_message_t msg;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	/* open/create file for writing */
	if ((fd = open(argv[1], O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) == -1) {
		logmsg(LOG_ERROR, "Unable to open file '%s'", argv[1]);
		return IOTD_ERROR_FILE_OPEN_FAIL;
	}

	if (fstat(fd, &sb) == -1) {
		logmsg(LOG_ERROR, "fstat() failed");
		return IOTD_ERROR_FILE_STAT_FAIL;
	}

	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	chan = lc_channel_new(ctx, MY_HARDCODED_CHANNEL);
	lc_channel_bind(sock, chan);
	lc_channel_join(chan);

	for (lc_msg_init(&msg);;lc_msg_free(&msg)) {
		lc_msg_recv(sock, &msg);
		logmsg(LOG_DEBUG, "message received");	
		f = msg.data;
		if (!map) { /* we have our first packet, so create the map */
			if (ftruncate(fd, f->size) != 0) {
				err_print(0, errno, "ftruncate()");
				ret = IOTD_ERROR_MMAP_FAIL;
				goto socket_close;
			}
			map = mmap(NULL, f->size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
			if (map == MAP_FAILED) {
				logmsg(LOG_ERROR, "mmap() failed: %s", strerror(errno));
				ret = IOTD_ERROR_MMAP_FAIL;
				goto socket_close;
			}
		}
		logmsg(LOG_DEBUG, "opcode: %i", (int)f->op);
		logmsg(LOG_DEBUG, "offset: %lld", (long long)f->off);
		logmsg(LOG_DEBUG, "length: %lld", (long long)f->len);

		/* write some data */
		memcpy(map + f->off, f->data, f->len);
		bwrit += f->len;
		logmsg(LOG_DEBUG, "bwritten: %lld", (long long)bwrit);
		logmsg(LOG_DEBUG, "filesize: %lld", (long long)f->size);

		if (f->size <= bwrit) { /* enough data, are we done? */
			logmsg(LOG_DEBUG, "checking hash...");
			hash(fhash, map, f->size);

			/* TEMP: print hash */
			for (int i = 0; i < HASHSIZE; ++i) {
				printf("%02x", ((unsigned char *)fhash)[i]);
			}
			printf("\n");
			for (int i = 0; i < HASHSIZE; ++i) {
				printf("%02x", ((unsigned char *)f->hash)[i]);
			}
			printf("\n");
			if (memcmp(fhash, f->hash, HASHSIZE) == 0) break;
		}
		msync(map, f->size, MS_ASYNC);
	}
	msync(map, f->size, MS_SYNC);
	munmap(map, f->size);
	lc_msg_free(&msg);
socket_close:
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	close(fd);
	
	return ret;
}
