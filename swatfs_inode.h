/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#ifndef __SWATFS_INODE_H__
#define __SWATFS_INODE_H__

#include "swatfs_types.h"

/* Read an inode from the file metadata region of the disk.
 *
 * inum: The number of the inode to read.
 * inode: A pointer to a struct inode that this function will fill in.
 *
 * Returns: 0 on success,
 *      -EINVAL if the inode number is not valid.
 *      -EIO if the block containing the inode could not be read. */
int inode_read(uint32_t inum, struct inode *inode);


/* Write an inode to the file metadata region of the disk.
 *
 * inum: The number of the inode to write.
 * inode: A pointer to a struct inode that this function copy to the disk.
 *
 * Returns: 0 on success,
 *      -EINVAL if the inode number is not valid.
 *      -EIO if the block containing the inode could not be read or written. */
int inode_write(uint32_t inum, struct inode *inode);


/* Scans through the inode region of the disk and finds the first unused
 * inode, where "unused" means the link count is zero.
 *
 * inum: A pointer to an integer that the function will fill in with the result.
 *
 * Returns: 0 on success,
 *      -EIO if the disk could not be read.
 *      -ENOSPC if there are no free inodes left. */
int inode_find_free(uint32_t *inum);

#endif
