/**
 * Copyright 2017 Gokturk Yuksek
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

#include <ext2fs/ext2fs.h>
#include <string.h>
#include <errno.h>

#include "libe2ntropy.h"
#include "libentropy.h"

static inline int get_device_size(char *device_path, unsigned int blocksize,
				blk64_t *size)
{
	return ext2fs_get_device_size2(device_path, blocksize, size);
}

/**
 * Open an ext file system instance
 */
int e2ntropy_open(struct e2ntropy_ctx *ctx, const char *device_path)
{
	int flags = EXT2_FLAG_64BITS | EXT2_FLAG_JOURNAL_DEV_OK;
	int err;

	/* If the argument device_path is NULL, use the one from the ctx */
	if (!device_path)
		return -EINVAL;

	err = ext2fs_open(device_path, flags, 0, 0, unix_io_manager, &ctx->fs);
	if (err)
		return err;

	/* Store the device path in ctx */
	ctx->device_path = strdup(device_path);

	return 0;
}

void e2ntropy_close(struct e2ntropy_ctx *ctx)
{
	ext2fs_close(ctx->fs);
	free(ctx->device_path);
	memset(ctx, 0, sizeof(*ctx));
}

int e2ntropy_iter_init(struct e2ntropy_ctx *ctx, struct e2ntropy_iter *iter)
{
	ext2_filsys fs = ctx->fs;
	int err;

	memset(iter, 0, sizeof(*iter));
	iter->ctx = ctx;
	iter->bg_flags = -1;
	iter->bg_offset_next = 1; /* libe2fs gets angry with block #0 */

	/* Determine maximum number of blocks possible */
	err = get_device_size(ctx->device_path, fs->blocksize,
			&iter->max_blocks);
	if (err)
		return err;

	/* Read the block bitmaps into memory */
	ext2fs_read_block_bitmap(fs);
}

int e2ntropy_iter_next(struct e2ntropy_iter *iter, unsigned buf_size, void *buf)
{
	struct e2ntropy_ctx *ctx = iter->ctx;
	ext2_filsys fs = ctx->fs;
	blk64_t block;
	int err;

	/* Buffer size must be equal to the blocksize */
	/* TODO: This limitation can be removed using a bounce buffer */
	if (buf_size != fs->blocksize)
		return -EINVAL;

	iter->bg_offset = iter->bg_offset_next;
try_next_bg:
	/* Check for the block group boundary */
	if (iter->bg_index >= fs->group_desc_count)
		return -ERANGE;

	/*
	 * If bg_flags isn't initialized, we haven't processed anything
	 * from this block group yet.
	 *
	 * We need to reset bg_flags every time we switch to a different bg
	 */
	if (iter->bg_flags == -1)
		if (ext2fs_has_group_desc_csum(fs))
			iter->bg_flags = ext2fs_bg_flags(fs, iter->bg_index);
		else
			iter->bg_flags = 0;

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
	if ((iter->bg_flags & EXT2_BG_BLOCK_UNINIT) == EXT2_BG_BLOCK_UNINIT) {
		iter->bg_index++;
		iter->bg_offset = 0;
		iter->bg_flags = -1;

		/* Try the next bg */
		goto try_next_bg;
	}

try_next_block:
	/* If we have exhausted all the blocks in this bg, move on */
	if (iter->bg_offset >= fs->super->s_clusters_per_group) {
		iter->bg_index++;
		iter->bg_offset = 0;
		iter->bg_flags = -1;

		/* Try the next bg */
		goto try_next_bg;
	}

	/* We finally have a block group, look for the unused blocks */
	block = e2ntropy_iter_block_index(iter);
	if (block >= iter->max_blocks)
		return -ERANGE;

	/* If this block is marked as used, try the next block */
	if (ext2fs_test_block_bitmap2(fs->block_map, block)) {
		iter->bg_offset++;
		goto try_next_block;
	}

	err = io_channel_read_blk64(fs->io, block, 1, buf);
	if (err)
		return err;
	iter->bg_offset_next = iter->bg_offset + 1;

	return 0;
}
