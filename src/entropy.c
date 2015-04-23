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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

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
	double entropy, p, logp;
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
	do {
		bytes_read = read(fd, buf, pagesize);
		if (bytes_read) {
			for (i = 0; i < bytes_read; i++)
				frequency_table[((unsigned char*)buf)[i]]++;
			symbol_count += bytes_read;
		}
	} while(bytes_read > 0);
	free(buf);
	close(fd);

	/* Calculate entropy */
	entropy = 0.0;
	for (i = 0; i < 256; i++) {
		/* Skip symbols with 0 frequency */
		if (!frequency_table[i])
			continue;
		p = (double)frequency_table[i] / (double)symbol_count;
		logp = log2(p);
		entropy -= p * logp;
	}
	fprintf(stdout, "%f\n", entropy);

	return 0;
}
