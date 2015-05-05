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

#define in_use(m, x)	(ext2fs_test_bit ((x), (m)))

static inline int ext2fs_has_group_desc_csum(ext2_filsys fs)
{
        return EXT2_HAS_RO_COMPAT_FEATURE(fs->super,
					EXT4_FEATURE_RO_COMPAT_GDT_CSUM |
					EXT4_FEATURE_RO_COMPAT_METADATA_CSUM);
}

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
	char *block_bitmap = NULL, *buf = NULL;
	char *device_path;
	blk64_t block, block_itr, j;
	int block_nbytes;
	struct entropy_ctx ctx;
	unsigned long i;
	int err;

	if (argc != 2)
		usage(argv[0]);
	device_path = argv[1];

	/* Open the file system */
	err = ext2fs_open(device_path, flags, 0, 0, unix_io_manager, &fs);
	if (err) {
		fprintf(stderr, "Unable to open device: ", device_path);
		return err;
	}
	/* Determine first data block */
	block_itr = EXT2FS_B2C(fs, fs->super->s_first_data_block);
	/* Read the block bitmaps into memory */
	ext2fs_read_block_bitmap(fs);
	/* Bytes needed for the block bitmap of a group */
	block_nbytes = EXT2_CLUSTERS_PER_GROUP(fs->super) / 8;
	/* Allocate space for per-group block bitmaps */
	block_bitmap = malloc(block_nbytes);
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
		/* Read the group's block bitmap */
		err = ext2fs_get_block_bitmap_range2(fs->block_map, block_itr,
						block_nbytes << 3,
						block_bitmap);
		if (err) {
			fprintf(stderr, "Unable to get block bitmap range"
				" (%llu - %llu): Aborting.", block_itr,
				block_itr + fs->super->s_clusters_per_group);
			goto out;
		}
		/*
		 * Iterate over the block bitmap and calculate
		 * the entropy of unused blocks
		 */
		for (j = 0; j < fs->super->s_clusters_per_group << 3; j++) {
			if (!in_use(block_bitmap, j)) {
				block = (i * fs->super->s_clusters_per_group) + j;
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
				libentropy_calculate(&ctx);
				fprintf(stdout, "%llu, %f\n",
					block, ctx.ec_entropy);
			}
		}
		block_itr += fs->super->s_clusters_per_group;
	}

out:
	free(buf);
	free(block_bitmap);
	ext2fs_close(fs);
	return err;
}