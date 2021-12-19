/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2019-2021 Brett Sheffield <brett@gladserv.com> */

#include "err.h"
#include "iot.h"
#include "log.h"
#include "chan.h"
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
#include <net/if.h>
#include <arpa/inet.h>

#define PROGRAM_NAME "iotupc"

static pthread_t tchecksum;
static pthread_t twriter;
static pthread_mutex_t dataready;
static int complete = 0;
static int fd = 0;
static char *map = NULL;
static size_t maplen = 0;
static unsigned char filehash[HASHSIZE];
static lc_ctx_t *ctx = NULL;
static lc_socket_t * sock = NULL;
static lc_channel_t * chan = NULL;

void cleanup();
void sigint_handler(int signo);
int thread_checksum(void *arg);
int thread_writer(void *arg);

void cancel_checksum_thread()
{
	complete = 1;
	pthread_mutex_unlock(&dataready);
}

void cleanup()
{
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	close(fd);
}

void sigint_handler(int signo)
{
	cancel_checksum_thread();
}

/* do all checksumming here in a separate thread to ensure it doesn't
 * slow down reading from receive buffer */
int thread_checksum(void *arg)
{
	unsigned char fhash[HASHSIZE];

	pthread_mutex_lock(&dataready); /* wait here until writer says go */

	while (!complete) {
		hash_generic(fhash, HASHSIZE, (unsigned char *)map, maplen);

		for (int i = 0; i < HASHSIZE; ++i) {
			printf("%02x", (unsigned char)fhash[i]);
		}
		printf("\n");
		for (int i = 0; i < HASHSIZE; ++i) {
			printf("%02x", (unsigned char)filehash[i]);
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
	struct iot_frame_t *f = NULL;
	char buf[sizeof (iot_frame_t)];

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
	while (!complete) {
		lc_socket_recv(sock, buf, sizeof (iot_frame_t), 0);
		f = (iot_frame_t *)buf;
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

		/* write some data */
		memcpy(map + f->off, f->data, f->len);
		bwrit += f->len;

		if (f->size <= bwrit + binit) { /* enough data */
			pthread_mutex_unlock(&dataready); /* begin checksumming */
		}
		msync(map, f->size, MS_ASYNC);
	}

exit_writer:
	cancel_checksum_thread();

	/* sync file to disk and unmap */
	munmap(map, maplen);

	return ret;
}

int main(int argc, char **argv)
{
	struct sockaddr_in6 addr;
	int ret = 0, ifindex = 0;

	if (argc < 3 || argc > 4) {
		fprintf(stderr, "usage: %s <file> <group> [<interface>]\n", argv[0]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	if (get_channel(argv[2], &addr) == -1) {
		logmsg(LOG_ERROR, "Group '%s' not valid", argv[2]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	if (argc > 3) {
		ifindex = if_nametoindex(argv[3]);
		if (ifindex == 0) {
			logmsg(LOG_ERROR, "Interface '%s' not found", argv[3]);
			return IOTD_ERROR_IF_NODEV;
		}
	}

	/* join our multicast channel */
	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	if (lc_socket_bind(sock, ifindex) == -1) {
		logmsg(LOG_ERROR, "Cannot bind to interface '%s'", argv[2]);
		return IOTD_ERROR_IF_NODEV;
	}
	chan = lc_channel_init(ctx, &addr);
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

	cleanup();

	return ret;
}
