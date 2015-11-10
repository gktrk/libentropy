/**
 * Copyright 2015 Gokturk Yuksek
 *
 * This file is part of libentropy.
 *
 * libentropy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libentropy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with libentropy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef  __LIBENTROPY_H__
#define  __LIBENTROPY_H__

#include <stdlib.h>

typedef union {
	double r_float;
} libentropy_result_t;

typedef enum {
	LIBENTROPY_ALGO_SHANNON,
	LIBENTROPY_ALGO_CHISQ,
} libentropy_algo_t;

enum {
	LIBENTROPY_STATUS_SUCCESS,
	LIBENTROPY_STATUS_FP_ERROR,
	LIBENTROPY_STATUS_UNKNOWN_ALGO,
};

struct entropy_ctx {
	unsigned long long ec_freq_table[256];
	unsigned long long ec_symbol_count;
};

void libentropy_update_ctx(struct entropy_ctx *ctx,
			const void *buf, size_t buf_len);
libentropy_result_t libentropy_calculate(struct entropy_ctx *ctx,
					libentropy_algo_t algo, int *err);
#endif /*__LIBENTROPY_H__*/
