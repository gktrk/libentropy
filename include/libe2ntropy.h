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

#ifndef  __LIBE2NTROPY_H__
#define  __LIBE2NTROPY_H__

#include <ext2fs/ext2fs.h>

#include "libentropy.h"

struct e2ntropy_ctx {
	char *device_path;
	ext2_filsys fs;
};

struct e2ntropy_iter {
	struct e2ntropy_ctx *ctx;
	unsigned long bg_index;
	blk64_t bg_offset;
	blk64_t bg_offset_next;
	int bg_flags;
	blk64_t max_blocks;
	char *buf;
	unsigned long long buf_len;
};

static inline unsigned int e2ntropy_iter_blocksize(struct e2ntropy_ctx *ctx)
{
	return ctx->fs->blocksize;
}

static inline blk64_t e2ntropy_iter_block_index(struct e2ntropy_iter *iter)
{
	return (iter->bg_index * iter->ctx->fs->super->s_clusters_per_group) +
		iter->bg_offset;
}

extern int e2ntropy_open(struct e2ntropy_ctx *ctx, const char *device_path);
extern void e2ntropy_close(struct e2ntropy_ctx *ctx);
extern int e2ntropy_iter_init(struct e2ntropy_ctx *ctx,
			struct e2ntropy_iter *iter);
extern const char *entropy_iter_get_buffer(struct e2ntropy_iter *iter,
					int *err);
extern int e2ntropy_iter_next(struct e2ntropy_iter *iter,
		struct entropy_batch_request *req);

#endif /*__LIBE2NTROPY_H__*/
