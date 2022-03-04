/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#include <errno.h>
#include <stdio.h>

#include "swatfs_blockbitmap.h"
#include "swatfs_disk.h"

#define BLOCK_FREE (0)
#define BLOCK_INUSE (1)

static uint8_t cache[SWATFS_BSIZE];

int blockbitmap_cache_init(void) {
    if (disk_read_block(SWATFS_BLOCKBITMAP, cache)) {
        return -errno;
    }

    return 0;
}

int blockbitmap_acquire_freeblock(uint32_t *bnum) {
    int i;

    /* Scan to find the first free block, mark it as in use, and set the
     * result value. */
    for (i = 0; i < (disk_size_blocks() - SWATFS_DATASTART); ++i) {
        if (cache[i] == BLOCK_FREE) {
            cache[i] = BLOCK_INUSE;
            *bnum = i + SWATFS_DATASTART;
            fprintf(stderr, "DEBUG:  ---ALLOCATING NEW BLOCK: %u\n", *bnum);
            if (disk_write_block(SWATFS_BLOCKBITMAP, cache)) {
                cache[i] = BLOCK_FREE;
                return -errno;
            }
            disk_sync();
            return 0;
        }
    }

    /* We found no free blocks. */
    errno = ENOSPC;
    return -errno;
}

int blockbitmap_release_block(uint32_t bnum) {
    fprintf(stderr, "DEBUG:  ---RELEASING BLOCK: %u\n", bnum);
    cache[bnum - SWATFS_DATASTART] = BLOCK_FREE;

    if (disk_write_block(SWATFS_BLOCKBITMAP, cache)) {
        return -errno;
    }

    disk_sync();
    return 0;
}
