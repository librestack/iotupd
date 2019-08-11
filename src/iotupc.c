/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "err.h"
#include "iot.h"
#include "log.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <librecast.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wolfssl/ssl.h>

#define PROGRAM_NAME "iotupc"

static pthread_t tchecksum;
static pthread_t twriter;
static pthread_mutex_t dataready;
static int complete = 0;
static struct iot_frame_t *f = NULL;
static int fd = 0;
static char *map = NULL;
static size_t maplen = 0;
static char filehash[HASHSIZE];
static lc_ctx_t *ctx = NULL;
static lc_socket_t * sock = NULL;
static lc_channel_t * chan = NULL;
static lc_message_t msg;

void sigint_handler(int signo);
void cleanup();
void terminate();
int thread_checksum(void *arg);
int thread_writer(void *arg);

void sigint_handler(int signo)
{
	terminate();
}

void cleanup()
{
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	close(fd);
}

void terminate()
{
	complete = 1;
	pthread_mutex_unlock(&dataready);
	pthread_cancel(tchecksum);
	pthread_cancel(twriter);
	pthread_join(tchecksum, NULL);
	pthread_join(twriter, NULL);
	cleanup();
	_exit(0);
}

/* do all checksumming here in a separate thread to ensure it doesn't
 * slow down reading from receive buffer */
int thread_checksum(void *arg)
{
	byte fhash[HASHSIZE];

	pthread_mutex_lock(&dataready); /* wait here until writer says go */

	while (complete != 1) {
		logmsg(LOG_DEBUG, "checking hash...");
		hash(fhash, map, maplen);

		for (int i = 0; i < HASHSIZE; ++i) {
			printf("%02x", ((unsigned char *)fhash)[i]);
		}
		printf("\n");
		for (int i = 0; i < HASHSIZE; ++i) {
			printf("%02x", ((unsigned char *)filehash)[i]);
		}
		printf("\n");

		if (memcmp(fhash, filehash, HASHSIZE) == 0) break;
	}

	return 0;
}

int thread_writer(void *arg)
{
	int ret = 0;
	u_int64_t binit = 0;
	u_int64_t bwrit = 0;
	struct stat sb;

	/* open/create file for writing */
	if ((fd = open(arg, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) == -1) {
		logmsg(LOG_ERROR, "Unable to open file '%s'", arg);
		ret = IOTD_ERROR_FILE_OPEN_FAIL;
		goto exit_writer;
	}

	if (fstat(fd, &sb) == -1) {
		logmsg(LOG_ERROR, "fstat() failed");
		ret = IOTD_ERROR_FILE_STAT_FAIL;
		goto exit_writer;
	}

	/* determine size on disk, to see how much data we already have */
	binit = 512 * sb.st_blocks;
	logmsg(LOG_DEBUG, "file already contains: %lld bytes", (long long)binit);

	/* receive data and write to map */
	for (lc_msg_init(&msg); complete == 0; lc_msg_free(&msg)) {
		lc_msg_recv(sock, &msg);
		logmsg(LOG_DEBUG, "message received");	
		f = msg.data;
		if (!map) { /* we have our first packet, so create the map */
			maplen = f->size;
			memcpy(&filehash, f->hash, HASHSIZE);
			if (ftruncate(fd, f->size) != 0) {
				err_print(0, errno, "ftruncate()");
				ret = IOTD_ERROR_MMAP_FAIL;
				goto exit_writer;
			}
			map = mmap(NULL, f->size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
			if (map == MAP_FAILED) {
				logmsg(LOG_ERROR, "mmap() failed: %s", strerror(errno));
				ret = IOTD_ERROR_MMAP_FAIL;
				goto exit_writer;
			}
		}
		logmsg(LOG_DEBUG, "opcode: %i", (int)f->op);
		logmsg(LOG_DEBUG, "offset: %lld", (long long)f->off);
		logmsg(LOG_DEBUG, "length: %lld", (long long)f->len);

		/* write some data */
		memcpy(map + f->off, f->data, f->len);
		bwrit += f->len;
		logmsg(LOG_DEBUG, "received: %lld bytes", (long long)bwrit);
		logmsg(LOG_DEBUG, "filesize: %lld bytes", (long long)f->size);

		if (f->size <= bwrit + binit) { /* enough data */
			pthread_mutex_unlock(&dataready); /* begin checksumming */
		}
		msync(map, f->size, MS_ASYNC);
	}

exit_writer:
	pthread_cancel(tchecksum);
	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	/* join our multicast channel */
	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	chan = lc_channel_new(ctx, MY_HARDCODED_CHANNEL);
	lc_channel_bind(sock, chan);
	lc_channel_join(chan);

	/* checksum thread will not start until this mutex released */
	pthread_mutex_init(&dataready, NULL);
	pthread_mutex_lock(&dataready);

	/* start threads */
	pthread_create(&tchecksum, NULL, (void *)&thread_checksum, NULL);
	pthread_create(&twriter, NULL, (void *)&thread_writer, argv[1]);

	/* catch SIGINT in main thread */
	signal(SIGINT, sigint_handler);

	/* when checksum thread returns, cancel writer and clean up */
	pthread_join(tchecksum, NULL);
	pthread_cancel(twriter);
	pthread_join(twriter, NULL);
	pthread_mutex_destroy(&dataready);

	/* sync file to disk and unmap */
	msync(map, f->size, MS_SYNC);
	munmap(map, f->size);

	cleanup();

	return ret;
}
