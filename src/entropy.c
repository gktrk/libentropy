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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

extern char *optarg;
extern int opting, opterr, optopt;

static void usage(const char *pname) {
	fprintf(stdout, "Usage: %s [-b blocksize] [-h] [filename]\n", pname);
	exit(-1);
}

int
main(int argc, char *argv[])
{
	int fd;
	void *buf;
	size_t bytes_read = 0;
	long i;
	unsigned long long frequency_table[256] = {0};
	unsigned long long symbol_count = 0;
	unsigned long long blocksize = 0;
	unsigned long long offset = 0;
	unsigned long long remaining = 0;
	unsigned long long read_size;
	double p, logp;
	struct entropy_ctx ctx;
	int c;
	char *tmp;
	const long pagesize = sysconf(_SC_PAGESIZE);

	while ((c = getopt(argc, argv, "b:h")) != -1) {
		switch (c) {
		case 'b':
			blocksize = strtoull(optarg, &tmp, 0);
			/*
			 * if optarg is '\0', no number is specified
			 * if (*tmp) isn't '\0', then stroull() found an
			 *   invalid character in the string
			 */
			if ((optarg[0] == '\0') || (*tmp != '\0')) {
				fprintf(stderr, "Invalid blocksize: %s\n",
					optarg);
				usage(argv[0]);
			}
			/*
			 * if return value is ULLONG_MAX and errno is
			 *   set to ERANGE, overflow happened
			 */
			if ((blocksize == ULLONG_MAX)) {
				perror("Invalid blocksize");
				usage(argv[0]);
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
		};
	}

	if ((optind == argc) ||
		((optind < argc) && (!strncmp(argv[optind], "-", 1))))
		fd = STDIN_FILENO;
	else
		fd = open(argv[optind++], O_RDONLY);
	if (fd == -1) {
		perror("Cannot open file");
		return errno;
	}

	/* Process one page of data at a time */
	buf = malloc(pagesize);
	memset(&ctx, 0, sizeof(struct entropy_ctx));
	ctx.ec_algo = LIBENTROPY_ALGO_SHANNON;
	do {
		/* Reset remaining at the start of each fresh block */
		if (blocksize && !remaining)
			remaining = blocksize;
		/* Determine read size */
		if ((blocksize) && (remaining < pagesize))
			read_size = remaining;
		else
			read_size = pagesize;
		/* Read data */
		bytes_read = read(fd, buf, read_size);
		/* Get some bookkeeping done */
		if (blocksize) {
			remaining -= bytes_read;
			offset += bytes_read;
		}

		/* Update frequencies etc. */
		libentropy_update_ctx(&ctx, buf, bytes_read);

		/*
		 * If we are done with the block, calculate its entropy
		 * and print it.
		 */
		if (blocksize && !remaining) {
			libentropy_calculate(&ctx);
			if (ctx.ec_status == LIBENTROPY_STATUS_SUCCESS) {
				fprintf(stdout, "%llu, %f\n", offset,
					ctx.ec_entropy);
				memset(&ctx, 0, sizeof(struct entropy_ctx));
			} else {
				fprintf(stderr, "%s():%d: %s: %d\n",
					__func__, __LINE__,
					"Entropy calculation failed",
					ctx.ec_status);
				free(buf);
				close(fd);
				return -1;
			}
		}
	} while(bytes_read > 0);
	free(buf);
	close(fd);

	/* Calculate entropy */
	if (!blocksize) {
		libentropy_calculate(&ctx);
		if (ctx.ec_status == LIBENTROPY_STATUS_SUCCESS)
			fprintf(stdout, "%f\n", ctx.ec_entropy);
	}

	return 0;
}
