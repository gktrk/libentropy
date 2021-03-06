/**
 * Copyright 2015,2017 Gokturk Yuksek
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
#include "libe2ntropy.h"

#include <ext2fs/ext2fs.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *pname)
{
	fprintf(stderr, "Usage: %s <device path>\n", pname);
	exit(-1);
}

int main(int argc, char *argv[])
{
	struct e2ntropy_ctx e2ctx;
	struct e2ntropy_iter e2iter;
	struct entropy_batch_request *req = NULL;
	char *device_path;
	unsigned long i;
	blk64_t block;
	double entropy, chisq;
	double entropy_min = - 1, chisq_max = -1;
	int err;

	if ((argc < 2) || (argc > 4))
		usage(argv[0]);
	device_path = argv[1];
	if (argc > 2) {
		entropy_min = atof(argv[2]);
		if ((entropy_min < 0) || (entropy_min > 8.0)) {
			fprintf(stderr, "Invalid minimum entropy: %f\n",
				entropy_min);
			return -1;
		}
	}
	if (argc > 3) {
		chisq_max = atof(argv[3]);
		if (chisq_max < 0)
			fprintf(stderr, "Invalid maximum chisq: %f\n",
				chisq_max);
			return -1;
	}

	/* Open the file system */
	err = e2ntropy_open(&e2ctx, device_path);
	if (err) {
		fprintf(stderr, "Unable to open device: ", device_path);
		return err;
	}

	/* Init the iterator */
	err = e2ntropy_iter_init(&e2ctx, &e2iter);
	if (err) {
		fprintf(stderr, "%s():%d: e2ntropy_iter_init() failed\n",
			__func__, __LINE__);
		goto out;
	}

	req = libentropy_alloc_batch_request(2, &err);
	if (!req) {
		fprintf(stderr, "%s():%d: libentropy_alloc_batch_request()"
			" failed\n", __func__, __LINE__);
		goto out;
	}
	req->algos[0] = LIBENTROPY_ALGO_SHANNON;
	req->algos[1] = LIBENTROPY_ALGO_CHISQ;

	while (!(err = e2ntropy_iter_next(&e2iter, req))) {
		entropy = req->results[0].r_float;
		chisq = req->results[1].r_float;

		if ((entropy_min > 0) &&
			(entropy < entropy_min))
			continue;
		if ((chisq_max > 0) &&
			(chisq > chisq_max))
			continue;

		fprintf(stdout, "%llu, %f, %f\n",
			block = e2ntropy_iter_block_index(&e2iter),
			entropy, chisq);
	}

out:
	libentropy_free_batch_request(req);
	e2ntropy_close(&e2ctx);
	return err;
}
