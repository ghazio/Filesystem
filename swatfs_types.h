/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#ifndef __SWATFS_TYPES_H__
#define __SWATFS_TYPES_H__

#include <inttypes.h>
#include <time.h>

/* Special constants - verified at FS mount time or mounting fails. */
#define SWATFS_MAGIC (0xFEEDF00D)
#define SWATFS_VERSION (2)

/* The size of a disk block. */
#define SWATFS_BSIZE (4096)

/* The maximum length of a name in a directory entry.  This was
 * chosen to make a struct dirent be an even 256 bytes. */
#define SWATFS_NAMELEN (252)

/* The blocks are arranged as:
 *    Block zero is reserved for catching errors.
 *    Block one is the superblock.
 *    Block two is the free block map.
 *    Blocks SWATFS_INODESTART through (SWATFS_DATASTART - 1) are inodes.
 *    Blocks starting from SWATFS_DATASTART are data. */

#define SWATFS_SUPERBLOCK (1)

#define SWATFS_BLOCKBITMAP (2)

/* The first block number that contains inodes. */
#define SWATFS_INODESTART (3)

/* The first block number that contains general data. */
#define SWATFS_DATASTART (7)

/* The number of direct pointers (block numbers) in an inode. */
#define SWATFS_DIRECTPTR (4)

/* This is just here for convenience.  It's the maximum number of blocks that
 * a file can use if it uses all of the direct pointers and a full indirect block. */
#define SWATFS_MAXFILEBLOCKS (SWATFS_DIRECTPTR + (SWATFS_BSIZE / sizeof(uint32_t)))

/* A structure for file system metadata. These are checked at FS mount time to
 * ensure that the disk we're mounting actually conforms to our FS format. */
struct superblock {
    uint32_t magic;
    uint32_t version;
};

/* This structure represents an inode, which stores the metadata for one file.
 * The size of this inode should be 64 bytes. */
struct inode {
    /* Number of times this inode is linked in to a directory (hard links).
     * You can assume that for any inode other than the root directory, this
     * value will be either 0 (free inode) or 1 (inode in use). */
    uint32_t links;

    /* The user ID of the file's owner. */
    uint32_t uid;

    /* The group ID of the group the file belongs to. */
    uint32_t gid;

    /* The file's mode is a bitwise OR'd value that encodes:
     * 1) whether the file is a regular file or directory (S_IFREG vs. S_IFDIR)
     * 2) what the access permissions are for the file (read/write/execute) for
     * the file's owner/group/world. */
    uint32_t mode;

    /* The size of the file, in bytes. */
    uint32_t size;

    /* The number of data blocks allocated to the file. */
    uint32_t blocks;

    /* The time this file was last modified. */
    struct timespec mtime;

    /* Block pointers. */
    uint32_t direct[SWATFS_DIRECTPTR];
    uint32_t indirect;
};

/* This structure represents one entry in a directory.
 * It maps a name to an inode number (inum).
 * 256 bytes. */
struct dirent {
    uint32_t inum;
    char name[SWATFS_NAMELEN];
};

#endif
