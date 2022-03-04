/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "swatfs_disk.h"
#include "swatfs_inode.h"

/* The number of inodes we can pack into one block. */
#define INODESPERBLOCK (SWATFS_BSIZE / sizeof(struct inode))

/* Helper to determine the block number and offset within a block when
 * looking for a particular inode. */
static int compute_inode(uint32_t inum, uint32_t *bnum, uint32_t *boffset) {
    /* Determine which block this inode lives in. */
    *bnum = (inum / INODESPERBLOCK) + SWATFS_INODESTART;

    /* Invalid inode block number. */
    if (*bnum >= SWATFS_DATASTART) {
        errno = EINVAL;
        return -errno;
    }

    /* Determine where within the block the inode is. */
    *boffset = inum % INODESPERBLOCK;

    return 0;
}

int inode_read(uint32_t inum, struct inode *inode) {
    uint32_t blocknum, blockoffset;
    struct inode block[SWATFS_BSIZE / sizeof(struct inode)];

    if (compute_inode(inum, &blocknum, &blockoffset)) {
        return -errno;
    }

    fprintf(stderr, "i_read: block %u offset is %u\n", blocknum, blockoffset);

    if (disk_read_block(blocknum, block)) {
        return -errno;
    }

    memcpy(inode, block + blockoffset, sizeof(struct inode));

    return 0;
}

int inode_write(uint32_t inum, struct inode *inode) {
    uint32_t blocknum, blockoffset;
    struct inode block[SWATFS_BSIZE / sizeof(struct inode)];

    if (compute_inode(inum, &blocknum, &blockoffset)) {
        return -errno;
    }

    fprintf(stderr, "i_write: block %u offset is %u\n", blocknum, blockoffset);

    if (disk_read_block(blocknum, block)) {
        return -errno;
    }

    memcpy(block + blockoffset, inode, sizeof(struct inode));

    if (disk_write_block(blocknum, block)) {
        return -errno;
    }

    return 0;
}

int inode_find_free(uint32_t *inum) {
    uint32_t bnum;

    for (bnum = SWATFS_INODESTART; bnum < SWATFS_DATASTART; ++bnum) {
        struct inode inode_block[INODESPERBLOCK];
        int i;

        if (disk_read_block(bnum, inode_block)) {
            return -errno;
        }

        for (i = 0; i < INODESPERBLOCK; ++i) {
            if (inode_block[i].links == 0) {
                *inum = (INODESPERBLOCK * (bnum - SWATFS_INODESTART)) + i;
                return 0;
            }
        }
    }

    errno = ENOSPC;
    return -errno;
}
