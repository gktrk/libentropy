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

#include "libentropy.h"
#include <math.h>

static double shannon_entropy(unsigned long long freq_table[256],
			unsigned long long symbol_count,
			int *err)
{
	double entropy = 0.0;
	double p, logp;
	unsigned i;

	for (i = 0; i < 256; i++) {
		/* Skip symbols with 0 frequency */
		if (!freq_table[i])
			continue;
		p = (double)freq_table[i] / (double)symbol_count;
		logp = log2(p);
		entropy -= p * logp;
	}
	if (isfinite(entropy))
		*err = LIBENTROPY_STATUS_SUCCESS;
	else
		*err = LIBENTROPY_STATUS_FP_ERROR;

	return entropy;
}

void libentropy_update_ctx(struct entropy_ctx *ctx,
			const void *buf, size_t buf_len)
{
	size_t i;

	if (!buf_len)
		return;

	for (i = 0; i < buf_len; i++)
		ctx->ec_freq_table[((unsigned char*)buf)[i]]++;
	ctx->ec_symbol_count += buf_len;
}

void libentropy_calculate(struct entropy_ctx *ctx)
{
	switch (ctx->ec_algo) {
	case LIBENTROPY_ALGO_SHANNON:
		ctx->ec_entropy = shannon_entropy(ctx->ec_freq_table,
						ctx->ec_symbol_count,
						&ctx->ec_status);
		break;
	default:
		ctx->ec_status = LIBENTROPY_STATUS_UNKOWN_ALGO;
	};
}
