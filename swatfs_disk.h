/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#ifndef __SWATFS_DISK_H__
#define __SWATFS_DISK_H__

#include <inttypes.h>

/* Open a disk image and verify that it looks like a SWATFS file system. */
int disk_open(const char *filename);

/* Issue writes to flush all data to disk and block until it completes. */
int disk_sync(void);

/* Sync the disk and close it cleanly for shutdown. */
int disk_close(void);

/* Read a block from the disk.
 *
 * block: the number of the block to read.
 * buf: a location to store the data that gets read.  The area of memory this
 *      points to should be the size of a disk block (SWATFS_BSIZE).
 *
 * Returns: 0 on success,
 *      -EIO if the disk block cannot be read. */
int disk_read_block(uint32_t block, void *buf);

/* Write a block to the disk.
 *
 * block: the number of the block to write.
 * buf: a pointer to the data that should be written. The area of memory this
 *      points to should be the size of a disk block (SWATFS_BSIZE).
 *
 * Returns: 0 on success,
 *      -EIO if the disk block cannot be written. */
int disk_write_block(uint32_t block, void *buf);

/* Returns the size of the disk, in bytes. */
size_t disk_size_bytes(void);

/* Returns the size of the disk, in number of blocks. */
uint32_t disk_size_blocks(void);

#endif
