/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#ifndef __SWATFS_BLOCKMAP_H__
#define __SWATFS_BLOCKMAP_H__

#include "swatfs_types.h"

/* Read and initialize the blockbitmap. */
int blockbitmap_cache_init(void);

/* Find the next free block on the disk and fill it in at the location pointed
 * to by bnum.
 *
 * bnum: a location to store the block ID of the resulting free block.
 *
 * Returns: 0 on success,
 *      -ENOSPC if there are no free blocks left. */
int blockbitmap_acquire_freeblock(uint32_t *bnum);

/* Marks the block identified by bnum as free. */
int blockbitmap_release_block(uint32_t bnum);

#endif
