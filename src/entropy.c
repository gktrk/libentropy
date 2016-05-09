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

#define _LARGEFILE64_SOURCE

#include "config.h"
#include "libentropy.h"
#include "entropy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>

extern char *optarg;
extern int opting, opterr, optopt;

/* Check if the 8-bit number is a power of 2 */
static inline unsigned __is_pow2_u8(unsigned char v)
{
	unsigned char r = 0;

	/* Count the number of 1's in the number */
	while(v) {
		if (v & 0x01)
			r++;
		v >>= 1;
	}

	/* If only one bit is set, it's a power of 2 */
	return (r == 1);
}

static void usage(const char *pname) {
	fprintf(stdout, "Usage: %s [-b blocksize] [-h] [-l size limit]"
		" [-s skip offset] [-m metric]"
		" [--bfd-bin-size size[=1]] [filename]\n"
		"\tMetrics: entropy[default], chisq, bfd\n", pname);
	exit(-1);
}

static unsigned long long parse_ull(const char *str, int *err)
{
	unsigned long long ret;
	char *tmp;

	ret = strtoull(str, &tmp, 0);
	/*
	 * if str is '\0', no number is specified
	 * if (*tmp) isn't '\0', then stroull() found an
	 *   invalid character in the string
	 */
	if ((str[0] == '\0') || (*tmp != '\0'))
		*err = -EINVAL;
	/*
	 * if return value is ULLONG_MAX and errno is
	 *   set to ERANGE, overflow happened
	 */
	if ((ret == ULLONG_MAX))
		*err = -ERANGE;
	*err = 0;
	return ret;
}

static libentropy_algo_t parse_metric(const char *str, int *err)
{
	*err = 0;
	if (strcmp(str, "entropy") == 0)
		return LIBENTROPY_ALGO_SHANNON;
	else if (strcmp(str, "chisq") == 0)
		return LIBENTROPY_ALGO_CHISQ;
	else if (strcmp(str, "bfd") == 0)
		return LIBENTROPY_ALGO_BFD;
	else
		*err = -1;
	/* Assume entropy metric, in case caller doesn't check err */
	return LIBENTROPY_ALGO_SHANNON;
}

static void set_default_opts(struct entropy_opts *opts)
{
	opts->blocksize = 0;
	opts->size_limit = 0;
	opts->skip_offset = 0;
	opts->algo = LIBENTROPY_ALGO_SHANNON;

	opts->bfd_bin_size = 1;
}

static int parse_args(int argc, char * const argv[], struct entropy_opts *opts)
{
	int c, err = 0;
	int option_index;
	unsigned i, j;

	enum {
		LONG_OPT_BFD_BIN_SIZE = 256,
	};
	const struct option long_options[] = {
		{
			.name = "bfd-bin-size",
			.has_arg = required_argument,
			.flag = 0,
			.val = LONG_OPT_BFD_BIN_SIZE,
		},
		{ 0, 0, 0, 0, },
	};

	if (!opts)
		return -1;
	set_default_opts(opts);

	while ((c = getopt_long(argc, argv, "b:hl:m:s:", long_options,
						&option_index)) != -1) {
		switch (c) {
		case 'b':
			opts->blocksize = parse_ull(optarg, &err);
			if (err) {
				fprintf(stderr, "Invalid blocksize (%s): %s\n",
					optarg, strerror(err));
				usage(argv[0]);
			}
			break;
		case 'l':
			opts->size_limit = parse_ull(optarg, &err);
			if (err) {
				fprintf(stderr, "Invalid size limit (%s):"
					" %s\n",
					optarg, strerror(err));
				usage(argv[0]);
			}
			break;
		case 'm':
			opts->algo = parse_metric(optarg, &err);
			if (err) {
				fprintf(stderr, "Invalid metric (%s)\n",
					optarg);
				usage(argv[0]);
			}
			break;
		case 's':
			opts->skip_offset = parse_ull(optarg, &err);
			if (err) {
				fprintf(stderr, "Invalid skip offset (%s):"
					" %s\n",
					optarg, strerror(err));
				usage(argv[0]);
			}
			break;
		case LONG_OPT_BFD_BIN_SIZE:
			opts->bfd_bin_size = (unsigned char)
				(parse_ull(optarg, &err) & 0xFF);
			if (err || (!__is_pow2_u8(opts->bfd_bin_size))) {
				fprintf(stderr, "Invalid bfd bin size (%s)\n",
					optarg);
				usage(argv[0]);
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
		};
	}

	if ((optind == argc) ||
		((optind < argc) && (!strncmp(argv[optind], "-", 1)))) {
		opts->file_count = 1;
		opts->fds = calloc(opts->file_count, sizeof(*opts->fds));
		opts->fds[0] = STDIN_FILENO;
	} else {
		opts->file_count = argc - optind;
		opts->fds = calloc(opts->file_count, sizeof(*opts->fds));
		if (!opts->fds) {
			err = errno;
			perror("Unable to allocate mem for fds");
			return err;
		}

		for (i = 0; i < opts->file_count; i++)
		{
			opts->fds[i] = open(argv[optind++], O_RDONLY);
			if (opts->fds[i] == -1) {
				fprintf(stderr, "%s():%d: Unable to open file"
					" %s: %s\n", __func__, __LINE__,
					argv[optind - 1], strerror(err));
				err = errno;
				for (j = 0; j < i; j++)
					close(opts->fds[j]);
				free(opts->fds);
				return err;
			}
		}
	}

	return err;
}

static int print_result(const libentropy_result_t result,
			libentropy_algo_t algo, unsigned long long offset,
			int offset_flag, unsigned char bfd_bin_size)
{
	const unsigned long long *bfd;
	unsigned long long sum;
	unsigned i, j;
	int err = 0;

	switch (algo) {
	case LIBENTROPY_ALGO_SHANNON:
	case LIBENTROPY_ALGO_CHISQ:
		if (offset_flag)
			fprintf(stdout, "%llu, %f\n", offset,
				result.r_float);
		else
			fprintf(stdout, "%f\n", result.r_float);
		break;
	case LIBENTROPY_ALGO_BFD:
		bfd = result.r_ptr;
		for (i = 0; i < (256 - bfd_bin_size); i += bfd_bin_size) {
			sum = 0;
			for (j = 0; j < bfd_bin_size; j++)
				sum += bfd[i + j];
			fprintf(stdout, "%llu,", sum);
		}
		/* Handle the last iteration outside the loop */
		sum = 0;
		for (j = 0; j < bfd_bin_size; j++)
			sum += bfd[i + j];
		fprintf(stdout, "%llu\n", sum);
		break;
	default:
		err = -1;
	};

	return err;
}

static int process_file(int fd, unsigned long long blocksize,
			unsigned long long size_limit,
			unsigned long long skip_offset,
			libentropy_algo_t algo,
			unsigned char bfd_bin_size)
{
	struct entropy_ctx ctx;
	void *buf;
	size_t bytes_read = 0;
	size_t total_bytes_read = 0;
	unsigned long long offset = 0;
	unsigned long long remaining = 0;
	unsigned long long read_size;
	libentropy_result_t result;
	int err;
	const long pagesize = sysconf(_SC_PAGESIZE);

	/* Handle skip offset */
	if (skip_offset) {
#if HAVE_LSEEK64
		err = (int)lseek64(fd, skip_offset, SEEK_CUR);
#else
		err = (int)lseek(fd, skip_offset, SEEK_CUR);
#endif
		if (err == -1) {
			perror("Cannot seek in file");
			return errno;
		}
		offset = skip_offset;
	}

#ifdef HAVE_POSIX_FADVISE
	/* Advise kernel that we will be reading sequantially */
	posix_fadvise(fd, skip_offset, 0, POSIX_FADV_SEQUENTIAL);
#endif

	/* Process one page of data at a time */
	buf = malloc(pagesize);
	memset(&ctx, 0, sizeof(struct entropy_ctx));
	do {
		/* If we hit the file size limit, break out of loop */
		if ((size_limit) && (total_bytes_read >= size_limit))
			break;
		/* Reset remaining at the start of each fresh block */
		if (blocksize && !remaining)
			remaining = blocksize;
		/* Determine read size */
		if ((blocksize) && (remaining < pagesize))
			read_size = remaining;
		else
			read_size = pagesize;
		/*
		 * Take size limit into account
		 *
		 * If the next read will end up reading more than
		 * what we want, adjust the read size properly
		 */
		if ((size_limit) &&
			((total_bytes_read + read_size) > size_limit))
			read_size -= (total_bytes_read + read_size) %
				size_limit;
		/* Read data */
		bytes_read = read(fd, buf, read_size);
		/* Get some bookkeeping done */
		if (blocksize)
			remaining -= bytes_read;
		offset += bytes_read;
		total_bytes_read += bytes_read;

		/* Update frequencies etc. */
		libentropy_update_ctx(&ctx, buf, bytes_read);

		/*
		 * If we are done with the block, calculate its entropy
		 * and print it.
		 */
		if (blocksize && !remaining) {
			result = libentropy_calculate(&ctx, algo, &err);
			if (err == LIBENTROPY_STATUS_SUCCESS) {
				print_result(result, algo, offset, 1,
					bfd_bin_size);
				memset(&ctx, 0, sizeof(struct entropy_ctx));
			} else {
				fprintf(stderr, "%s():%d: %s: %d\n",
					__func__, __LINE__,
					"Entropy calculation failed",
					err);
				free(buf);
				close(fd);
				return -1;
			}
		}
	} while(bytes_read > 0);
	free(buf);

	/* Calculate entropy */
	if (!blocksize) {
		result = libentropy_calculate(&ctx, algo, &err);
		if (err == LIBENTROPY_STATUS_SUCCESS)
			print_result(result, algo, 0, 0, bfd_bin_size);
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	struct entropy_opts opts;
	unsigned i;
	int err;

	err = parse_args(argc, argv, &opts);
	if (err)
		return err;

	for (i = 0; i < opts.file_count; i++)
	{
		err = process_file(opts.fds[i], opts.blocksize, opts.size_limit,
				opts.skip_offset, opts.algo, opts.bfd_bin_size);
		close(opts.fds[i]);
	}

	free(opts.fds);
	return 0;
}
