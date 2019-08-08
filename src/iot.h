#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct iot_val_t {
	size_t	len;
	void *	data;
} iot_val_t;

typedef struct iot_file_t {
	off_t	size;			/* Total size, in bytes */
} iot_file_t;

typedef struct iot_frame_t {
	uint8_t		op;		/* opcode */
	iot_val_t	data;		/* data */
} __attribute__((__packed__)) iot_frame_t;
