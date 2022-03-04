/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "swatfs_disk.h"
#include "swatfs_types.h"

/* Memory mapped file representing our disk. */
static uint8_t *disk;

/* The maximum size of our disk, in bytes. */
static size_t disk_size;

/* Pointer to the superblock. */
static struct superblock *superblock;

int disk_open(const char *filename) {
    int fd;
    struct stat status;

    /* Open the file. */
    if ((fd = open(filename, O_RDWR)) == -1) {
        return -errno;
    }

    /* Get the file's size with stat. */
    if (fstat(fd, &status)) {
        return -errno;
    }
    disk_size = status.st_size;

    if (disk_size > (SWATFS_BSIZE * (SWATFS_BSIZE + SWATFS_DATASTART))) {
        fprintf(stderr, "WARNING: Disk size exceeds maximum addressable.\n");
        disk_size = SWATFS_BSIZE * (SWATFS_BSIZE + SWATFS_DATASTART);
    }

    /* Map the file into memory. */
    if ((disk = mmap(NULL, disk_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                     fd, 0)) == MAP_FAILED) {
        int errsv = errno;
        close(fd);
        errno = errsv;
        return -errno;
    }

    /* The FD is no longer needed now that the file is mapped in memory. */
    if (close(fd)) {
        int errsv = errno;
        munmap(disk, disk_size);
        errno = errsv;
        return -errno;
    }

    superblock = (struct superblock *) (disk + (SWATFS_SUPERBLOCK * SWATFS_BSIZE));

    /* Verify the magic number indicates our file system. */
    if (superblock->magic != SWATFS_MAGIC) {
        fprintf(stderr, "FS superblock's magic number (0x%08X) doesn't match "
                        "expected magic number (0x%08X).\n",
                        superblock->magic, SWATFS_MAGIC);
        munmap(disk, disk_size);
        errno = EIO;
        return -errno;
    }

    /* Verify that the version matches. */
    if (superblock->version != SWATFS_VERSION) {
        fprintf(stderr, "FS superblock's version number (%u) doesn't match "
                        "expected version number (%u).\n",
                        superblock->version, SWATFS_VERSION);
        munmap(disk, disk_size);
        errno = EIO;
        return -errno;
    }

    /* Verify that total disk size is a multiple of the block size. */
    if (disk_size % SWATFS_BSIZE != 0) {
        fprintf(stderr, "FS file size (%zu) is not a multiple of its reported"
                        " block size (%u).\n", disk_size, SWATFS_BSIZE);
        munmap(disk, disk_size);
        errno = EIO;
        return -errno;
    }

    return 0;
}

int disk_sync(void) {
    if (msync(disk, disk_size, MS_SYNC)) {
        return -errno;
    }

    return 0;
}

int disk_close(void) {
    if (munmap(disk, disk_size)) {
        return -errno;
    }

    return 0;
}

int disk_read_block(uint32_t block, void *buf) {
    size_t offset = block * SWATFS_BSIZE;

    fprintf(stderr, "READ BLOCK num %u, (offset %zu, disk size %zu)\n", block, offset, disk_size);
    if (block == 0) {
        printf("Block 0 is reserved for error checking, and you just tried to access it.  You're probably attempting to access uninitialized block numbers?\n");
        exit(1);
    }

    if (offset >= disk_size) {
        errno = EIO;
        return -errno;
    }

    /* Read from the mmaped disk. */
    memcpy(buf, &disk[offset], SWATFS_BSIZE);

    return 0;
}

int disk_write_block(uint32_t block, void *buf) {
    size_t offset = block * SWATFS_BSIZE;

    fprintf(stderr, "WRITE BLOCK num %u, (offset %zu, disk size %zu)\n", block, offset, disk_size);
    if (block == 0) {
        printf("Block 0 is reserved for error checking, and you just tried to access it.  You're probably attempting to access uninitialized block numbers?\n");
        exit(1);
    }

    if (offset >= disk_size) {
        errno = EIO;
        return -errno;
    }

    /* Write to the mmaped disk. */
    memcpy(&disk[offset], buf, SWATFS_BSIZE);

    return 0;
}

size_t disk_size_bytes(void) {
    return disk_size;
}

uint32_t disk_size_blocks(void) {
    return disk_size / SWATFS_BSIZE;
}
