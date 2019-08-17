/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

/* disable warning in wolfssl - `./configure --enable-harden` does not work */
#define WC_NO_HARDEN /* FIXME */
#define WOLFSSL_SHA3

#include <wolfssl/wolfcrypt/sha3.h>

#define HASHSIZE WC_SHA3_224_DIGEST_SIZE

/* IPv6 path discovery isn't much use for multicast and
 * we don't want to receive a bunch of Packet Too Big messages
 * so we'll use a fixed MTU of 1280 - headers + extensions => ~1200 */
#define MTU_FIXED 1194

/* with no congestion control, we can't just send full throttle
 * may need to have several streams depending on device and recv buffer
 * for now, just put a delay between packets */
//#define PKT_DELAY 500 /* microseconds */

#define MY_HARDCODED_CHANNEL "wibble" /* FIXME */

typedef struct iot_file_t {
	off_t	size;			/* Total size, in bytes */
} iot_file_t;

typedef struct iot_frame_t {
	uint8_t		op;				/* opcode */
	u_int64_t	size;				/* full file size */
	u_int64_t	off;				/* offset */
	size_t		len;				/* length of this chunk */
	unsigned char	hash[HASHSIZE];			/* SHA3 hash of file */
	char 		data[MTU_FIXED];		/* data */
} __attribute__((__packed__)) iot_frame_t;

unsigned char * hash(unsigned char digest[HASHSIZE], char *data, size_t len);
