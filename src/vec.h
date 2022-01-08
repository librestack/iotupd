/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2020-2022 Brett Sheffield <bacs@librecast.net> */

#ifndef _VEC_H
#define _VEC_H 1

#include <limits.h>
#include <immintrin.h>
#include <stdint.h>

#define VECTOR_SZ 16
#define VECTOR_BITS VECTOR_SZ * CHAR_BIT

typedef unsigned char u8x16 __attribute__ ((vector_size (VECTOR_SZ)));
typedef union { __m128i v; u8x16 u8; } vec_t;

void    vec_inc_epi8(vec_t *v, size_t idx);
void    vec_dec_epi8(vec_t *v, size_t idx);
uint8_t vec_get_epi8(vec_t *v, size_t idx);
void    vec_set_epi8(vec_t *v, size_t idx, uint8_t val);

#endif /* _VEC_H */
