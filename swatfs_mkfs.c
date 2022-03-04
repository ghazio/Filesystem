/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "swatfs_types.h"

/* This program formats a disk image for our basic CS45 file system. */

int main(int argc, char **argv) {
    int fd;
    struct stat status;
    uint8_t *disk;
    struct superblock *superblock;
    struct inode *root;
    uint32_t bcount;

    if (argc != 2) {
        printf("Usage: %s <file to format>\n", argv[0]);
        exit(1);
    }

    printf("An inode is %zu bytes.\n", sizeof(struct inode));
    printf("A dirent is %zu bytes.\n", sizeof(struct dirent));

    /* Open the file. */
    if ((fd = open(argv[1], O_RDWR)) == -1) {
        perror("open");
        exit(1);
    }

    /* Get the file's size with stat. */
    if (fstat(fd, &status)) {
        perror("fstat");
        exit(1);
    }

    /* Verify that total disk size is a multiple of the block size. */
    if (status.st_size % SWATFS_BSIZE != 0) {
        printf("Block size doesn't evenly divide file size!\n");
        exit(1);
    }

    /* Map the file into memory. */
    if ((disk = mmap(NULL, status.st_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fd, 0)) == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(1);
    }

    /* The FD is no longer needed now that the file is mapped in memory. */
    if (close(fd)) {
        perror("close");
        munmap(disk, status.st_size);
        exit(1);
    }

    /* Nuke the whole file. */
    memset(disk, 0, status.st_size);

    /* Set the initial superblock values. */
    superblock = (struct superblock *) (disk + (SWATFS_SUPERBLOCK * SWATFS_BSIZE));
    superblock->magic = SWATFS_MAGIC;
    superblock->version = SWATFS_VERSION;

    /* Set inode 0, at the start of block 3 to be a directory for root (/) */
    root = (struct inode *) (disk + (SWATFS_INODESTART * SWATFS_BSIZE));
    root->links = 2;
    root->uid = getuid();
    root->gid = getgid();
    root->mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP;
    root->size = 0;
    root->blocks = 0;
    clock_gettime(CLOCK_REALTIME, &root->mtime);

    /* Flush to disk. */
    if (msync(disk, status.st_size, MS_SYNC)) {
        perror("msync");
        exit(1);
    }

    /* Unmap the file. */
    if (munmap(disk, status.st_size)) {
        perror("munmap");
        exit(1);
    }

    bcount = status.st_size / SWATFS_BSIZE;

    /* Print status. */
    printf("Disk formatted successfully:\n");
    printf("   Total size: %zu bytes (%u blocks)\n", status.st_size, bcount);

    if ((bcount - SWATFS_DATASTART) > SWATFS_BSIZE) {
        printf("WARNING: disk size exceeds block map capacity!\n");
    }

    return 0;
}
