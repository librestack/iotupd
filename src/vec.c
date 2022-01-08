/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2020-2022 Brett Sheffield <bacs@librecast.net> */

#include "vec.h"

/* set one element of a vector to value */
void vec_inc_epi8(vec_t *v, size_t idx)
{
	v[idx / VECTOR_BITS].u8[idx % VECTOR_BITS]++;
}

void vec_dec_epi8(vec_t *v, size_t idx)
{
	(v[idx / VECTOR_BITS].u8[idx % VECTOR_BITS])--;
}

uint8_t vec_get_epi8(vec_t *v, size_t idx)
{
	return v[idx / VECTOR_BITS].u8[idx % VECTOR_BITS];
}

void vec_set_epi8(vec_t *v, size_t idx, uint8_t val)
{
	v[idx / VECTOR_BITS].u8[idx % VECTOR_BITS] = val;
}
