/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2019-2022 Brett Sheffield <brett@librecast.net> */

#include "chan.h"
#include "err.h"
#include "iot.h"
#include "log.h"
#include "mld.h"
#include "progress.h"
#include <errno.h>
#include <fcntl.h>
#include <librecast.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/ioctl.h>

#define PROGRAM_NAME "iotupd"

static lc_ctx_t *ctx = NULL;
static lc_socket_t *sock = NULL;
static lc_channel_t *chan[MAX_CHANNELS] = {0};
static volatile int running = 1;
static int mld_enabled;
static int fd;
static char *map;
static struct stat sb;
size_t maplen;
size_t byt_in;
size_t byt_out;
sem_t semprogress;

static void sigint_handler(int signo)
{
	running = 0;
}

static void terminate()
{
	for (int i = 0; i < MAX_CHANNELS; i++) lc_channel_free(chan[i]);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	munmap(map, sb.st_size);
	close(fd);
	_exit(0);
}

int main(int argc, char **argv)
{
	struct sockaddr_in6 addr, a;
	struct iot_frame_t f;
	struct timespec pkt_delay = { 0, };
	const int on = 1;
	unsigned ifindex = 0, pkt_loop = 0;
	uint16_t len;
	ssize_t ret;
	mld_t *mld = NULL;
	pthread_t tid_progress;

	if (argc < 3 || argc > 6) {
		fprintf(stderr, "usage: %s <file> <group> [<delay>] [<interface>] [--mld]\n", argv[0]);
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

	if (get_channel(argv[2], &addr) == -1) {
		logmsg(LOG_ERROR, "Group '%s' not valid", argv[2]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	for (int argn = 3; argn < argc; argn++) {
		/* is this a packet delay? */
		if (sscanf(argv[argn], "%ld.%d", &pkt_delay.tv_nsec, &pkt_loop) >= 1) {
			logmsg(LOG_DEBUG, "packet delay %ld.%d", pkt_delay.tv_nsec, pkt_loop);
			continue;
		}
		if (strlen(argv[argn]) == 5 && !strcmp(argv[argn], "--mld")) {
			logmsg(LOG_DEBUG, "MLD triggering enabled");
			mld_enabled = 1;
			continue;
		}
		if (argc > argn) {
			ifindex = if_nametoindex(argv[argn]);
			logmsg(LOG_DEBUG, "binding to interface %s(%u)", argv[argn], ifindex);
			if (ifindex == 0) {
				logmsg(LOG_ERROR, "Interface '%s' not found", argv[argn]);
				return IOTD_ERROR_IF_NODEV;
			}
			continue;
		}
	}

	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);

	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	if (lc_socket_bind(sock, ifindex) == -1) {
		logmsg(LOG_ERROR, "Cannot bind to interface '%s'", argv[2]);
		return IOTD_ERROR_IF_NODEV;
	}
	lc_socket_setopt(sock, IPV6_MULTICAST_LOOP, &on, sizeof(on));

	for (int i = 0; i < MAX_CHANNELS; i++) {
		memcpy(&a, &addr, sizeof(struct sockaddr_in6));
		a.sin6_addr.s6_addr[15] += i;
		chan[i] = lc_channel_init(ctx, &a);
	}
	for (int i = 0; i < MAX_CHANNELS; i++) lc_channel_bind(sock, chan[i]);

	memset(&f, 0, sizeof(iot_frame_t));

	/* calculate file hash */
	hash_generic(f.hash, HASHSIZE, (unsigned char *)map, sb.st_size);

	/* ready to go, tell lwmon */
	lwmon_log("server_multicast", "start");

	/* set up MLD trigger */
	if (mld_enabled) {
		mld = mld_start(&running, ifindex, &addr);
		if (!mld) {
			ERROR("unable to start MLD (requires CAP_NET_RAW / root)");
			_exit(EXIT_FAILURE);
		}
	}
	pthread_create(&tid_progress, NULL, progress_update, NULL);

	unsigned long req = 0;
	while (running) {
		int channo = 0;
		for (int i = 0; i <= sb.st_size && running; i += MTU_FIXED) {
			int rc = 0;
			int flags = (channo) ? MLD_DONTWAIT : 0; /* only block on primary channel */
			/* only send if someone is listening */
			if (mld_enabled) {
				rc = mld_wait(mld, ifindex, lc_channel_in6addr(chan[channo]), flags);
			}
			/* don't overfill outbound send buffer */
			while (!rc) {
				int qlen;
				if (ioctl(lc_socket_raw(sock), TIOCOUTQ, &req) == -1)
					break;
				qlen = req / MTU_FIXED;
				if (qlen <= SENDQ_PKTS)
					break;
				if (pkt_delay.tv_nsec > 0)
					nanosleep(&pkt_delay, NULL);
				for (int i = 0; i < pkt_loop; i++) ;
			}
			f.size = htobe64(sb.st_size);
			f.off = htobe64(i);

			if ((i + MTU_FIXED) > sb.st_size) {
				len = sb.st_size - i;
			}
			else {
				len = MTU_FIXED;
			}
			f.len = htons(len);

			//logmsg(LOG_DEBUG, "sending %i - %i", i, (int)(i+f.len));

			memcpy(f.data, map + i, len);
			if (!rc) {
				ret = lc_channel_send(chan[channo], &f, sizeof(f), 0);
				if (ret != -1) byt_out += (size_t)ret;
			}
			channo++;
			if (channo >= MAX_CHANNELS) {
				channo = 0;
				f.seq = htons(ntohs(f.seq) + 1);
			}
		}
	}
	pthread_cancel(tid_progress);
	pthread_join(tid_progress, NULL);
	if (mld_enabled) mld_stop(mld);
	lwmon_log("server_multicast", "end");
	terminate();

	return 0;
}
