/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#ifndef __SWATFS_DIRECTORY_H__
#define __SWATFS_DIRECTORY_H__

#include <fuse.h>
#include <inttypes.h>

#include "swatfs_types.h"

/* Given a path, fill in the corresponding file's inode struct and inode
 * number.  This function will be used by just about every function, since
 * fuse passes paths to most of them. */
int directory_lookup(const char *path, struct inode *result, uint32_t *inum);

/* The functions below are directory-related functions that I found helpful.
 * You can implement and use these or ignore them, it's up to you. */

/* Add an entry to a directory.
 * dir_inum: The inode number of the directory.
 * ent_inum: The inode number of the new entry to add.
 * name: The name of the entry to add. */
//int directory_add(uint32_t dir_inum, uint32_t ent_inum, const char *name);

/* Remove an entry from a directory.
 * dir_inum: The inode number of the directory.
 * ent_inum: The inode number of the new entry to remove. */
//int directory_remove(uint32_t dir_inum, uint32_t ent_inum);

/* Read all the entires in a directory and use fuse's filler to fill them in.
 * inode: pointer to the directory's inode.
 * buf and filler: passed in from readdir.  Used by fuse to record entry
 *                 names. */
//int directory_read(struct inode *inode, void *buf, fuse_fill_dir_t filler);

#endif
