#include "iot.h"
/*
WOLFSSL_API int wc_InitSha3_224(wc_Sha3*, void*, int);
WOLFSSL_API int wc_Sha3_224_Update(wc_Sha3*, const byte*, word32);
WOLFSSL_API int wc_Sha3_224_Final(wc_Sha3*, byte*);
WOLFSSL_API void wc_Sha3_224_Free(wc_Sha3*);
WOLFSSL_API int wc_Sha3_224_GetHash(wc_Sha3*, byte*);
WOLFSSL_API int wc_Sha3_224_Copy(wc_Sha3* src, wc_Sha3* dst);
*/

byte * hash(byte digest[HASHSIZE], char *data, size_t len)
{
	wc_Sha3 sha;

	wc_InitSha3_224(&sha, NULL, INVALID_DEVID);
	wc_Sha3_224_Update(&sha, (unsigned char *)data, len);
	wc_Sha3_224_Final(&sha, digest);
	wc_Sha3_224_Free(&sha);

	return digest;
}
