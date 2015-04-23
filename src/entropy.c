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

static void usage(const char *pname) {
	fprintf(stdout, "Usage: %s <filename>\n", pname);
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
	double p, logp;
	struct entropy_ctx ctx;
	const long pagesize = sysconf(_SC_PAGESIZE);

	if (argc != 2)
		usage(argv[0]);

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("Cannot open file");
		return errno;
	}

	/* Process one page of data at a time */
	buf = malloc(pagesize);
	memset(&ctx, 0, sizeof(struct entropy_ctx));
	ctx.ec_algo = LIBENTROPY_ALGO_SHANNON;
	do {
		bytes_read = read(fd, buf, pagesize);
		libentropy_update_ctx(&ctx, buf, bytes_read);
	} while(bytes_read > 0);
	free(buf);
	close(fd);

	/* Calculate entropy */
	libentropy_calculate(&ctx);
	if (ctx.ec_status == LIBENTROPY_STATUS_SUCCESS)
		fprintf(stdout, "%f\n", ctx.ec_entropy);

	return 0;
}
