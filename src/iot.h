/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2019-2021 Brett Sheffield <brett@gladserv.com> */

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <librecast/crypto.h>

#define MAX_CHANNELS 3

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
	size_t		len;				/* length of this chunk */
	unsigned char	hash[HASHSIZE];			/* SHA3 hash of file */
	char 		data[MTU_FIXED];		/* data */
} __attribute__((__packed__)) iot_frame_t;
