#include <stddef.h>
#include <stdint.h>

#define	CRC_POLY_32	0xEDB88320L
#define	CRC_START_32	0xFFFFFFFFL

uint32_t	crc_32( const unsigned char *input_str, size_t num_bytes );
