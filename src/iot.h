/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2019-2022 Brett Sheffield <brett@librecast.net> */

#include <semaphore.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <librecast/crypto.h>

#define MAX_CHANNELS 1
#define LOSS_TOLERANCE 0.05 /* MAX PACKET LOSS % */
#define PKTS_STABILITY 10000    /* number of packets to receive without loss to consider stable */
#define SENDQ_PKTS 100 /* limit number of outbound packets in send queue */

/* IPv6 path discovery isn't much use for multicast and
 * we don't want to receive a bunch of Packet Too Big messages
 * so we'll use a fixed MTU of 1280 - headers + extensions => ~1200 */
#define MTU_FIXED 1194

typedef struct iot_file_t {
	off_t	size;			/* Total size, in bytes */
} iot_file_t;

typedef struct iot_frame_t {
	u_int16_t	seq;				/* sequence number */
	u_int64_t	size;				/* full file size */
	u_int64_t	off;				/* offset */
	u_int16_t	len;				/* length of this chunk */
	unsigned char	hash[HASHSIZE];			/* SHA3 hash of file */
	char 		data[MTU_FIXED];		/* data */
} __attribute__((__packed__)) iot_frame_t;

extern size_t maplen;
extern size_t byt_in;
extern size_t byt_out;
extern sem_t semprogress;
