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
#include <errno.h>

static double shannon_entropy(const unsigned long long freq_table[256],
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

/*
 * Chi square:
 *    X^2 = SUM { (Observed_i - Expected_i)^2 / (Expected_i) }
 *
 *    Expected_i = N * probability_i
 *    |  N: symbol count
 *
 * Note that we assume uniform distribution of symbols from a source
 * alphabet of 8 bits. Therefore each symbol has a probability of 1/256.
 *
 * Using this information, we can simplify the above equation to the following:
 *    X^2 = SUM { (Observed_i)^2 } / Expected - N
 *    |  Expected: N / 256
 */
static double chisq(const unsigned long long freq_table[256],
		unsigned long long symbol_count, int *err)
{
	const double N = (double)symbol_count;
	const double expected = N / 256.0;
	double sum = 0, ret;
	unsigned i;

	/* SUM { (Observed_i)^2 } */
	for (i = 0; i < 256; i++)
		sum += freq_table[i] * freq_table[i];
	ret = sum / expected - N;

	if (isfinite(ret))
		*err = LIBENTROPY_STATUS_SUCCESS;
	else
		*err = LIBENTROPY_STATUS_FP_ERROR;
	return ret;
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

libentropy_result_t libentropy_calculate(const struct entropy_ctx *ctx,
					libentropy_algo_t algo, int *err)
{
	libentropy_result_t result;

	switch (algo) {
	case LIBENTROPY_ALGO_SHANNON:
		result.r_float = shannon_entropy(ctx->ec_freq_table,
						ctx->ec_symbol_count, err);
		break;
	case LIBENTROPY_ALGO_CHISQ:
		result.r_float = chisq(ctx->ec_freq_table,
				ctx->ec_symbol_count, err);
		break;
	case LIBENTROPY_ALGO_BFD:
		/* No work is required for this one, we already have it */
		result.r_ptr = ctx->ec_freq_table;
		*err = LIBENTROPY_STATUS_SUCCESS;
		break;
	default:
		*err = LIBENTROPY_STATUS_UNKNOWN_ALGO;
	};

	return result;
}

int libentropy_batch(const struct entropy_ctx *ctx,
		struct entropy_batch_request *req)
{
	unsigned char i;

	for (i = 0; i < req->count; i++)
		req->results[i] = libentropy_calculate(ctx, req->algos[i],
						&req->errors[i]);

	return 0;
}

struct entropy_batch_request *
libentropy_alloc_batch_request(unsigned char count, int *err)
{
	struct entropy_batch_request *req = NULL;

	*err = -ENOMEM;
	req = malloc(sizeof(*req));
	if (!req)
		return NULL;
	req->count = count;

	req->algos = calloc(count, sizeof(libentropy_algo_t));
	if (!req->algos) {
		free(req);
		return NULL;
	}

	req->results = calloc(count, sizeof(libentropy_result_t));
	if (!req->results) {
		free(req->algos);
		free(req);
		return NULL;
	}

	req->errors = calloc(count, sizeof(int));
	if (!req->errors) {
		free(req->results);
		free(req->algos);
		free(req);
		return NULL;
	}

	*err = 0;
	return req;
}

void libentropy_free_batch_request(struct entropy_batch_request *req)
{
	if (req) {
		free(req->errors);
		free(req->results);
		free(req->algos);
	}
	free(req);
}
