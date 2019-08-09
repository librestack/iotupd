/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct iot_file_t {
	off_t	size;			/* Total size, in bytes */
} iot_file_t;

typedef struct iot_frame_t {
	uint8_t		op;		/* opcode */
	u_int64_t	size;		/* full file size */
	u_int64_t	off;		/* offset */
	size_t		len;		/* length of this chunk */
	char 		data[1200];	/* data */
} __attribute__((__packed__)) iot_frame_t;
