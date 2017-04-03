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
	ext2_filsys fs = NULL;
	int flags = EXT2_FLAG_64BITS | EXT2_FLAG_JOURNAL_DEV_OK;
	int bg_flags = 0;
	char *buf = NULL;
	char *device_path;
	blk64_t block, j;
	blk64_t max_blocks;
	int block_nbytes;
	struct entropy_ctx ctx;
	libentropy_result_t result;
	libentropy_algo_t algo;
	unsigned long i;
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
	err = ext2fs_open(device_path, flags, 0, 0, unix_io_manager, &fs);
	if (err) {
		fprintf(stderr, "Unable to open device: ", device_path);
		return err;
	}
	/* Determine maximum number of possbile blocks */
	err = ext2fs_get_device_size2(device_path, fs->blocksize, &max_blocks);
	if (err) {
		fprintf(stderr, "Unable to get device size: %s\n", device_path);
		goto out;
	}
	/* Read the block bitmaps into memory */
	ext2fs_read_block_bitmap(fs);
	/* Bytes needed for the block bitmap of a group */
	block_nbytes = EXT2_CLUSTERS_PER_GROUP(fs->super) / 8;
	/*
	 * Allocate space to hold unallocated file system
	 * blocks for entropy calculation
	 */
	buf = malloc(fs->blocksize);

	/* Iterate over block group */
	for (i = 0; i < fs->group_desc_count; i++) {
		/*
		 * Watch out for BLOCK_UNINIT
		 *
		 * If the block bitmap of the group is uninitialized,
		 * meaning it has never been touched by the file system,
		 * it's very likely that it's filled with 0s. Note
		 * that this may not necessarily be true for reformatting
		 * a used drive. We will skip those for now.
		 *
		 * Also keep in mind that this is an optional feature for
		 * mkfs and may not have been activated during formatting.
		 * So the absence of BLOCK_UNINIT does not guarantee
		 * that the blocks have been touched by the fs.
		 */
		if (ext2fs_has_group_desc_csum(fs))
			bg_flags = ext2fs_bg_flags(fs, i);
		if ((bg_flags & EXT2_BG_BLOCK_UNINIT) == EXT2_BG_BLOCK_UNINIT)
			continue;
		/*
		 * Iterate over the block bitmap and calculate
		 * the entropy of unused blocks
		 */
		for (j = 0; j < fs->super->s_clusters_per_group; j++) {
			block = (i * fs->super->s_clusters_per_group) + j;
			if (block >= max_blocks)
				break;
			if (!ext2fs_test_block_bitmap2(fs->block_map, block)) {
				err = io_channel_read_blk64(fs->io, block,
							1, buf);
				if (err) {
					fprintf(stderr,
						"Unable to read block: %llu\n",
						block);
					goto out;
				}
				memset(&ctx, 0, sizeof(struct entropy_ctx));
				libentropy_update_ctx(&ctx, buf, fs->blocksize);

				algo = LIBENTROPY_ALGO_SHANNON;
				result = libentropy_calculate(&ctx, algo, &err);
				entropy = result.r_float;
				algo = LIBENTROPY_ALGO_CHISQ;
				result = libentropy_calculate(&ctx, algo, &err);
				chisq = result.r_float;

				if ((entropy_min > 0) &&
					(entropy < entropy_min))
					continue;
				if ((chisq_max > 0) &&
					(chisq > chisq_max))
					continue;

				fprintf(stdout, "%llu, %f, %f\n",
					block, entropy, chisq);
			}
		}
	}

out:
	free(buf);
	ext2fs_close(fs);
	return err;
}
