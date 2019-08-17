/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "iot.h"

unsigned char * hash(unsigned char digest[HASHSIZE], char *data, size_t len)
{
	wc_Sha3 sha;

	wc_InitSha3_224(&sha, NULL, INVALID_DEVID);
	wc_Sha3_224_Update(&sha, (unsigned char *)data, len);
	wc_Sha3_224_Final(&sha, digest);
	wc_Sha3_224_Free(&sha);

	return digest;
}
