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
static lc_channel_t * chan[MAX_CHANNELS] = {0};
static uint64_t pkts;  /* total packets */
static uint64_t lost;  /* total lost packets */
static uint64_t apkts; /* total packets since last adjustment */
static uint64_t alost; /* packets lost since last adjustment */
static uint16_t actchans; /* active receive channels */

void cleanup();
void sigint_handler(int signo);
int thread_checksum(void *arg);
int thread_writer(void *arg);

void cancel_checksum_thread()
{
	complete = 1;
	pthread_mutex_unlock(&dataready);
	_exit(0);
}

void cleanup()
{
	for (int i = 0; i < MAX_CHANNELS; i++) lc_channel_free(chan[i]);
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


	while (!complete) {
		pthread_mutex_lock(&dataready); /* wait here until writer says go */
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
	u_int16_t last = UINT16_MAX;
	u_int16_t len, seq;
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
	binit = sb.st_size;
	logmsg(LOG_DEBUG, "file already contains: %lld bytes", (long long)binit);

	/* receive data and write to map */
	while (!complete) {
		lc_socket_recv(sock, buf, sizeof (iot_frame_t), 0);
		f = (iot_frame_t *)buf;
		seq = ntohs(f->seq);
		if (!map) { /* we have our first packet, so create the map */
			last = seq;
			maplen = (size_t)be64toh(f->size);
			logmsg(LOG_DEBUG, "receiving file of size %zu", maplen);
			memcpy(&filehash, f->hash, HASHSIZE);
			if (ftruncate(fd, maplen) != 0) {
				err_print(0, errno, "ftruncate()");
				ret = IOTD_ERROR_MMAP_FAIL;
				goto exit_writer;
			}
			map = mmap(NULL, maplen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
			if (map == MAP_FAILED) {
				logmsg(LOG_ERROR, "mmap() failed: %s", strerror(errno));
				ret = IOTD_ERROR_MMAP_FAIL;
				goto exit_writer;
			}
		}
		pkts++; apkts++;
		if (last + 1 < seq) { /* track packet loss */
			uint64_t gap = seq - last - 1;
			//logmsg(LOG_DEBUG, "last: %u seq: %u gap = %lu", last, seq, gap);
			lost += gap;
			alost += gap;
			//logmsg(LOG_DEBUG, "apkts=%lu, alost=%lu", apkts, alost);
			if (apkts && alost && actchans > 1) {
				double lrate = (double)alost / (double)apkts * 100;
				//logmsg(LOG_DEBUG, "loss rate = %0.2f %", lrate);
				if (lrate > LOSS_TOLERANCE) {
					alost = 0; apkts = 0; /* reset counters */
					lc_channel_part(chan[--actchans]);
					logmsg(LOG_DEBUG, "packet loss too high (%0.2f %), reducing to %u channels", lrate, actchans);
				}
			}
		}
#if 0
		if (apkts > PKTS_STABILITY * 10 && actchans < MAX_CHANNELS) {
			/* no packet loss, moar speed ! */
			apkts = 0;
			lc_channel_join(chan[actchans++]);
			logmsg(LOG_DEBUG, "no packet loss increasing to %u channels", actchans);
		}
#endif
		last = seq;

		/* write some data */
		len = ntohs(f->len);
		if (memcmp(map + be64toh(f->off), f->data, len)) {
			memcpy(map + be64toh(f->off), f->data, len);
			bwrit += len;
			//logmsg(LOG_DEBUG, "received: %lld bytes", (long long)bwrit);

			msync(map, maplen, MS_ASYNC);
		}
		if (maplen <= bwrit + binit) { /* enough data */
			pthread_mutex_unlock(&dataready); /* begin checksumming */
		}
	}

exit_writer:
	cancel_checksum_thread();

	/* sync file to disk and unmap */
	munmap(map, maplen);

	return ret;
}

int main(int argc, char **argv)
{
	struct sockaddr_in6 addr, a;
	float pcloss;
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

	/* join our multicast channel(s) */
	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	if (lc_socket_bind(sock, ifindex) == -1) {
		logmsg(LOG_ERROR, "Cannot bind to interface '%s'", argv[2]);
		return IOTD_ERROR_IF_NODEV;
	}
	for (int i = 0; i < MAX_CHANNELS; i++) {
		memcpy(&a, &addr, sizeof(struct sockaddr_in6));
		a.sin6_addr.s6_addr[15] += i;
		chan[i] = lc_channel_init(ctx, &a);
		lc_channel_bind(sock, chan[i]);
		/* join all channels - we'll back off if there's a problem */
		lc_channel_join(chan[actchans++]);
	}

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

	pcloss = (pkts) ? (float)lost / (float)pkts * 100: 0.00f;
	logmsg(LOG_DEBUG, "packets lost: %u / %llu (%0.2f %)", lost, pkts, pcloss);
	cleanup();

	return ret;
}
