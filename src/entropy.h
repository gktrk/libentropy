/**
 * Copyright 2016 Gokturk Yuksek
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

#ifndef  __ENTROPY_H__
#define  __ENTROPY_H__

#include <libentropy.h>

struct entropy_opts {
	int *fds;
	unsigned file_count;
	unsigned long long blocksize;
	unsigned long long size_limit;
	unsigned long long skip_offset;
	libentropy_algo_t algo;

	/* Options specific to Binary Frequency Distribution (bfd) */
	unsigned char bfd_bin_size;
};

#endif /*__ENTROPY_H__*/
