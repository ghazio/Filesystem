/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FUSE_USE_VERSION 30
#include <fuse.h>

#include "swatfs_blockbitmap.h"
#include "swatfs_directory.h"
#include "swatfs_disk.h"
#include "swatfs_inode.h"

/* These two functions make chopping up paths easier.  If given a path,
 * e.g., /dir1/dir2/dir3/filename
 *
 * dirname will give you the path of the directory that contains the file:
 * /dir1/dir2/dir3
 *
 * basename will give you the name of just the last component:
 * filename
 *
 * These two functions assume that path names are no longer than SWATFS_NAMELEN.
 *
 * See manual pages for more details about how these functions work:
 * man 3 dirname     man 3 basename
 *
 * For both of these, YOU ARE RESPONSIBLE for freeing the memory when you're
 * done with the string!*/
static char *easy_dirname(const char *path) {
    char pathcpy[SWATFS_NAMELEN+1];
    char *result;

    memset(pathcpy, 0, SWATFS_NAMELEN+1);
    strncpy(pathcpy, path, SWATFS_NAMELEN);

    result = strdup(dirname(pathcpy));
    return result;
}
static char *easy_basename(const char *path) {
    char pathcpy[SWATFS_NAMELEN+1];
    char *result;

    memset(pathcpy, 0, SWATFS_NAMELEN+1);
    strncpy(pathcpy, path, SWATFS_NAMELEN);

    result = strdup(basename(pathcpy));
    return result;
}

/* Shut down the file system. */
static void swatfs_destroy(void *unused) {
    if (disk_close()) {
        perror("disk_close");
        exit(1);
    }
    fprintf(stderr, "Disk image successfully closed.\n");
}

/* Get the attributes of a file. */
static int swatfs_getattr(const char *path, struct stat *status) {
    struct inode inode;
    uint32_t inum;

    fprintf(stderr, "DEBUG: swatfs_getattr, path: %s\n", path);

    /* Example: Find the file's inode based on the path. */
    errno = directory_lookup(path, &inode, &inum);
    if (errno != 0) {
      printf("inside dir lookup error\n");
        return errno;
    }

    // Fill in the status struct with info from the inode.
    status->st_nlink = inode.links;
    status->st_uid = inode.uid;
    status->st_gid = inode.gid;
    status->st_mode = inode.mode;
    status->st_size = inode.size;
    status->st_blocks = inode.blocks;
    status->st_mtim = inode.mtime;

    return 0;
}

/* Read the list of entries in a directory. */
static int swatfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi) {
    /* NOTE: you don't need to use offset or fi for anything. */
    fprintf(stderr, "DEBUG: swatfs_readdir, path: %s\n", path);
    struct inode dir_inode;
    uint32_t inum;
    // Find the directory's inode.
    errno = directory_lookup(path, &dir_inode, &inum);
    //if it's not a directory, set errno to ENOTDIR and return -errno
    if (errno != 0){
      errno = ENOTDIR;
      return -errno;
    }

      int entry_size = sizeof(struct dirent);
      int max_entries = 16;
      // int total_entries = dir_inode.size/256;
      int leftover_entries = (dir_inode.size % SWATFS_BSIZE)/entry_size;
    struct dirent block[SWATFS_BSIZE / sizeof(struct dirent)];
  //for each block in our dir_inode
    for (int i = 0; i < dir_inode.blocks; i++){
      //diskread each dirent struct into a block
      disk_read_block(dir_inode.direct[i],block);
      //if the block isn't the last block (ie, it is full)
      if(i < dir_inode.blocks-1){
        //iterate through each entry
        for (int j =0; j < max_entries; j++){
          //cse this weird
          //use fuse filler thing to add names to the resulting buffer (buf).
          filler (buf, block[j].name, NULL, 0);
        }
        //if it is the last block (ie. not full of entries)
      }else{
        //iterate through the entries
          for (int j =0; j < leftover_entries; j++){
            //use fuse filler thing to add names to the resulting buffer (buf).
            filler (buf, block[j].name, NULL, 0);
          }
        }
      }
    //Read the dirent structs out of its data blocks.  Use this weird
    //       fuse filler thing to add names to the resulting buffer (buf).
    //       filler(buf, name, NULL, 0)
    /* This ensures that . and .. appear in all directories. */
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    //directory_read(dir_inode, buf, filler);
    return 0;
}

//helper function to add a file/dir to block that take in a mode
// ADD
// 1) Find the index number (the next available spot)
// 2) fill in the inode num with function that finds inode
//      2.1) strcpy
// 3) update the inode size
// 4)write to the inode
//
// Cases: 1) just normal adding to a block thats already allocated
// 	   2) adding not in the first block (so adding in the last block)
// 	  3) adding if there is no block space left or no block allocated (inode.size is multiple of 4096 or inode.size=0)
// 		allocate new block (similar to write, and then add to inode.direct []
// 		update the metadata (size, blocks)

static int adder(const char *path, mode_t mode){
  struct inode inode;
  uint32_t inum;

  int index_num;
  uint32_t inum_new;
  char * temp= easy_dirname(path);
  char parent_path[SWATFS_NAMELEN];
  strcpy(parent_path,temp);
  printf("parent_path: %s\n", parent_path);

  errno = directory_lookup(parent_path, &inode, &inum);
  if(errno != 0){
    printf("inside second dir_lookup\n");
    return errno;
  }
  int leftover_bytes=inode.size%SWATFS_BSIZE;

  int block_index = inode.blocks-1;


  //check if we need to allocate more blocks
  //(inode.size is multiple of 4096 or inode.size=0)
  if(inode.size%SWATFS_BSIZE == 0 || inode.size ==0){
    printf("inside first if \n");
    block_index = inode.blocks;
    //allocate new block
    errno=blockbitmap_acquire_freeblock(&inode.direct[block_index]);
    if(errno!=0){
      return errno;
    }
    index_num = 0;
    inode.blocks++;

  }

  //if first block has space
  else{
    printf("inside else\n");
    index_num = ((inode.size % SWATFS_BSIZE) / sizeof(struct dirent));
  }

  struct dirent block[SWATFS_BSIZE / sizeof(struct dirent)];
  struct dirent new_dirent;
  //find a free inode
  errno = inode_find_free(&inum_new);
  if( errno != 0){
    printf("inside find free \n");
    return errno;
  }
  printf("after find free\n");

  //gets the last part of the path
  char* filename = easy_basename(path);

//read in the inode's block
  printf("before disk read\n");
  errno=disk_read_block(inode.direct[block_index], &block);
  if(errno!=0){
    printf("inside read_block erro\n");
    return errno;
  }

  //strcpy to the new inode
  strcpy(block[index_num].name,filename);
  block[index_num].inum = inum_new;

  free(filename);
  free (temp);
  inode.size = inode.size + sizeof(struct dirent);
  printf("after disk read\n");
  // memcpy(&block[index_num]+leftover_bytes,&new_dirent,sizeof( struct dirent));


  //write the block to the disk
  errno = disk_write_block(inode.direct[block_index],&block);
  if(errno!=0){
    printf("inside disk_write_block error\n");
    return errno;
  }
  printf("after disk write \n");
  errno = inode_write(inum,&inode);
  if(errno!=0){
    printf("inside first inode write\n");
    return errno;
  }
  printf("after inode first write \n");
  //allocate new inode
  struct inode new_inode;
  new_inode.links = 1;
  new_inode.uid = getuid();
  new_inode.gid = getgid();
  new_inode.mode = mode ;
  new_inode.size = 0;
  new_inode.blocks = 0;
  clock_gettime(CLOCK_REALTIME, &new_inode.mtime);
  for (int i =0; i< SWATFS_DIRECTPTR; i++){
    new_inode.direct[i] = 0;
  }
  new_inode.indirect = 0;
  errno = inode_write(inum_new, &new_inode);
  printf("after inode second write \n");

  if(errno!=0){
    printf("inside second inode write\n");
    return errno;
  }
  return 0;
}


/* Create a new directory. */
static int swatfs_mkdir(const char *path, mode_t mode) {
  printf("inside mkdir\n");
    fprintf(stderr, "DEBUG: swatfs_mkdir, path: %s\n", path);

    /* NOTE: when you use the 'mkdir' command in your shell, it will call
     * getattr on the path *first*.  It expects that getattr call to fail with
     * ENOENT, since you can't make a new directory if the specified path
     * already exists.  If your mkdir function never seems to get called, make
     * sure that swatfs_getattr and directory_lookup are setting errno to
     * ENOENT and returning -ENOENT for paths that don't actually exist. */

    return adder(path, mode | S_IFDIR);
}

/* NOTE: mkdir() and create() are VERY similar in what they do.  I would
 *       strongly suggest using a helper function for these.  In my
 *       implementation, they were entirely identical except that the mode
 *       was ORed with either S_IFDIR or S_IFREG. */

/* Create a new regular file. */
static int swatfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    fprintf(stderr, "DEBUG: swatfs_create, path: %s\n", path);

    return adder(path, mode | S_IFREG);
}

/* Read up to size bytes of data from a file, starting at the specified byte
 * offset, into the provided buffer (buf).
 * You can ignore the "fi" pointer if you want (I did). */
static int swatfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    /* NOTE: This function should return the number of bytes copied to buf.
     *       It may return fewer bytes than were asked for in size, and in
     *       some cases has no choice but to do so (e.g., if we reach the end
     *       of the file.  Returning 0 means "end of file".
     *
     * You do NOT have to satisfy a large read request all at once as long
     * as your return value correctly informs the user how much data you're
     * returning to them.  They will call read again if need be.
     */
     fprintf(stderr, "DEBUG: swatfs_read, path: %s, size: %lu, offset: %ld\n", path, size, offset);
     //block,
     struct inode dir_inode;
     uint32_t inum;
     int size_of_block=0;
     int end_of_block;
     //if offset is greater than the size of the file, then we have reached
     //the end of the file already and we return 0
     // Find the directory's inode.
     errno = directory_lookup(path, &dir_inode, &inum);
     if(errno!=0){
       return -errno;
     }

     int leftover_bytes = dir_inode.size % SWATFS_BSIZE;

     int last_block=dir_inode.blocks-1;

     if(offset>=dir_inode.size){

       return 0;
     }
     //we have found the inode of the file
    char block[SWATFS_BSIZE];
    int block_index= offset/SWATFS_BSIZE;
    if(block_index>3){

      uint32_t indirect_block[SWATFS_BSIZE / sizeof(uint32_t)];

      errno=disk_read_block(dir_inode.indirect,&indirect_block);

      if(errno!=0){
        return -errno;
      }

      block_index = block_index-4;
      errno=disk_read_block(indirect_block[block_index],block);

    }//then we look into the indirect
    else{

      errno=disk_read_block(dir_inode.direct[block_index],block);

      if(errno!=0){
        return -errno;
      }

    }

    off_t new_offset = offset % SWATFS_BSIZE;
    long blocks_used = offset / SWATFS_BSIZE;

    //end of file
    int eof = dir_inode.size - offset;
    //end of block
    int eob = SWATFS_BSIZE - new_offset;
    //if last block, only copy valid data
    if(blocks_used == last_block){
      eob = leftover_bytes - new_offset;
    }
    //end of size allocated
    int eos = size;
    //finding the minimun of the above value to calculate end_of_block
    if (eof < eob){

      end_of_block = eof;
    }else{

      end_of_block = eob;
    }
    if(eos < end_of_block){

      end_of_block = eos;
    }


    memcpy(buf,block + new_offset,end_of_block);
    return end_of_block;

}
// offset+size=size of the file that user wants
/* Write up to size bytes of data to a file, starting at the specified byte
 * offset, from the provided buffer (buf).
 * You can ignore the "fi" pointer. */
static int swatfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    /* NOTE: This function should return the number of bytes copied from buf to
     *       the file.  It may return fewer bytes than were asked for in size.
     *       It should not return 0.
     *
     *       I would STRONGLY suggest that you only write one block at a time.
     *       That is, even if the application asks you to write a huge amount
     *       of data at once, you fill in everything up to the next block and
     *       then return the amount that you wrote.  The application will call
     *       write again to write the rest, and dealing with just one block at
     *       a time will simplify your write implementation.
     *
     *       For example, let's say you have an empty file and the application
     *       calls write() and asks you to write 9000 bytes.  Just write the
     *       first SWATFS_BSIZE (4096) bytes and fill in one block.  When you
     *       return 4096, the application will know that it needs to call write
     *       again with the remaining (9000 - 4096 = 4904) bytes. */

    struct inode inode;
    uint32_t inum;

    fprintf(stderr, "DEBUG: swatfs_write, path: %s, size: %lu, offset: %ld\n", path, size, offset);

    /* Find the inode for the file we're writing. */
    if (directory_lookup(path, &inode, &inum)) {
        return -errno;
    }

    /* If there's a gap between the last existing byte and the offset we're
     * being asked to write, bail out. */
    if (offset > inode.size) {
        errno = ENOSYS;
        return -errno;
    }

    int block_index = offset/SWATFS_BSIZE;
    int size_final= SWATFS_BSIZE;
    uint32_t indirect_block[SWATFS_BSIZE / sizeof(uint32_t)];

    if(block_index==inode.blocks){

        if(block_index<4){

          errno=blockbitmap_acquire_freeblock(&inode.direct[block_index]);
          if(errno!=0){
            return -errno;
          }
          //direct block
        }
        else if(block_index==4){
          //we need to allocate a new array for indirect pointers


          errno=blockbitmap_acquire_freeblock(&inode.indirect);
          if(errno!=0){
            return -errno;
          }

          errno=disk_read_block(inode.indirect,&indirect_block);
          if(errno!=0){
            return -errno;
          }
          block_index=block_index-4;
          //acquire two blocks instead of one for the indirect pointer blocks
          errno=blockbitmap_acquire_freeblock(&indirect_block[block_index]);
          if(errno!=0){
            return -errno;
           }
           errno=disk_write_block(inode.indirect,&indirect_block);
           if(errno!=0){
             return -errno;
           }
          }
        else{
          //indirect


          errno=disk_read_block(inode.indirect,&indirect_block);
          if(errno!=0){
            return -errno;
          }
          block_index=block_index-4;
          //acquire a block
          errno=blockbitmap_acquire_freeblock(&indirect_block[block_index]);
          if(errno!=0){
            return -errno;
          }
          errno=disk_write_block(inode.indirect,&indirect_block);
          if(errno!=0){
            return -errno;
          }
        }

      inode.blocks++;
      inode_write(inum, &inode);

    }

    block_index=offset/SWATFS_BSIZE;
    char block[SWATFS_BSIZE];

    if(block_index<4){
    //calculate how much you want to copy
    errno =disk_read_block(inode.direct[block_index],block);
    if(errno!=0){
      return -errno;
    }
    size_final=SWATFS_BSIZE;
    if(size_final>size){
      size_final=size;
    }
    memcpy(block+(offset%SWATFS_BSIZE),buf,size_final);
    clock_gettime(CLOCK_REALTIME, &inode.mtime);

    if(inode.size < (offset+size_final)){
      inode.size = offset + size_final;
    }
    inode_write(inum, &inode);
    errno =disk_write_block(inode.direct[block_index],block);
    if(errno!=0){
        return -errno;
      }
    return size_final;
    }
    else{

      //first read in the indirect block
      errno=disk_read_block(inode.indirect,&indirect_block);
      if(errno!=0){
        return -errno;
      }

      errno =disk_read_block(indirect_block[block_index-4],block);
      if(errno!=0){
        printf("inside errno\n");
        return -errno;
      }

      int size_final=SWATFS_BSIZE;
      if(size_final>size){
        size_final=size;
      }

      memcpy(block+offset%SWATFS_BSIZE,buf,size_final);

      clock_gettime(CLOCK_REALTIME, &inode.mtime);
      if(inode.size < (offset+size_final)){
        inode.size = offset + size_final;
      }
      inode_write(inum, &inode);

      errno =disk_write_block(indirect_block[block_index-4],block);

      if(errno!=0){
          return -errno;
        }
      return size_final;
    }

}

/* Make a file smaller by shrinking its size.  This is commonly used to
 * empty a file and make it zero bytes.
 *
 * This function is provided for you, and you shouldn't need to make changes
 * to it, assuming you provide a 'directory_lookup' that matches my recommendation. */
static int swatfs_truncate(const char *path, off_t size) {
    struct inode inode;
    uint32_t first_remove;
    uint32_t inum;
    uint32_t indirect[SWATFS_BSIZE / sizeof(uint32_t)];  // A block of SWATFS_BSIZE bytes treated as an array of uint32_t's.
    uint32_t total_blocks;
    int i, read_indirect = 0;

    fprintf(stderr, "DEBUG: swatfs_truncate, path: %s, size: %lu\n", path, size);

    /* Determine the block pointer number of the first block that we're going to remove. */
    first_remove = size / SWATFS_BSIZE;

    /* If we're keeping a partial block, advance the first block to remove by one. */
    if ((size % SWATFS_BSIZE) > 0) {
        first_remove += 1;
    }

    /* Get the inode for this file. */
    if (directory_lookup(path, &inode, &inum)) {
        return -errno;
    }

    /* Read the indirect block so that we can access its block pointers if we might need it. */
    if (inode.blocks > SWATFS_DIRECTPTR) {
        read_indirect = 1;
        if (disk_read_block(inode.indirect, indirect)) {
            return -errno;
        }
    }

    /* For each block that we're removing, find the block number by reading the
     * block pointers from either direct or indirect pointers and then free
     * the block so that something else can use it later. */
    total_blocks = inode.blocks;
    for (i = first_remove; i < total_blocks; ++i) {
        uint32_t bnum;

        if (i < SWATFS_DIRECTPTR) {
            /* Read block num from direct and free it. */
            bnum = inode.direct[i];
            inode.direct[i] = 0;
        } else {
            /* Read block num from indirect and free it. */
            bnum = indirect[i - SWATFS_DIRECTPTR];
            indirect[i - SWATFS_DIRECTPTR] = 0;
        }

        /* Free the block. */
        if (blockbitmap_release_block(bnum)) {
            return -errno;
        }

        inode.blocks -= 1;
    }

    /* If we read the indirect block pointer disk block earlier, save it back. */
    if (read_indirect) {
        if (disk_write_block(inode.indirect, indirect)) {
            return -errno;
        }

        /* Free the indirect block if we no longer need it. */
        if (inode.blocks < SWATFS_DIRECTPTR) {
            if (blockbitmap_release_block(inode.indirect)) {
                return -errno;
            }
            inode.indirect = 0;
        }
    }

    /* Update the size of our inode and then write the inode changes to disk. */
    inode.size = size;
    if (inode_write(inum, &inode)) {
        return -errno;
    }

    /* Success */
    return 0;
}

// 1) find the item to remove (kinda like dir lookup where you scan for that item)
  //1.1 remove the item
// 2) find the last element in all blocks and move it (strcpy) to where you removed the first item (unless the first item is the final item in the block)
// 3) decrement node’s size by one dirent
// 4)write to the inode
static int remover(const char* path){
  struct inode inode;
  uint32_t inum;
  struct inode inode_parent;
  uint32_t inum_parent;

  char * temp= easy_dirname(path);
  char parent_path[SWATFS_NAMELEN];
  strcpy(parent_path,temp);
  //find the last element that is gonna get moved
  //check if we are removing the last element,
  //if it is the last one on its block, get rid of the block as well
  //otherwise, we delete the element and put the last element in its place
  if (directory_lookup(path, &inode, &inum)) {
      return errno;
  }
  if (directory_lookup(parent_path, &inode_parent, &inum_parent)) {
      return errno;
  }
  //ask kevin about deleting
  inode.links=0;
  uint32_t block_index=inode_parent.blocks-1;
  struct dirent block[SWATFS_BSIZE / sizeof(struct dirent)];

  int leftover_entries = (inode_parent.size % SWATFS_BSIZE)/sizeof(struct dirent);
  if(leftover_entries==0){
    leftover_entries=16;
  }
  int block_del, index_del,block_last, index_last;
  struct dirent* del_dirent;
  struct dirent* last_dirent;
  int max_entries = SWATFS_BSIZE/sizeof(struct dirent);
  for (int i = 0; i < inode_parent.blocks; i++){
    //diskread each dirent struct into a block
    errno=disk_read_block(inode_parent.direct[i],&block);
    if(errno!=0){
      return -errno;
    }
    //if the block isn't the last block (ie, it is full)
    if(i < inode_parent.blocks-1){
      //iterate through each entry
      for (int j =0; j < max_entries; j++){
        //if we find the struct dirent that we want to remove, save it
        if(inum==block[j].inum){
          block_del=i;
          index_del=j;
          del_dirent=&block[j];
        }
      }
      //if it is the last block (ie. not full of entries)
    }else{
      //iterate through the entries
        for (int j =0; j < leftover_entries; j++){
          if(inum==block[j].inum){
            block_del=i;
            index_del=j;
            del_dirent=&block[j];
          }
          if(j==leftover_entries-1){
            block_last=i;
            index_last=j;
            last_dirent=&block[j];
          }
        }
      }
  }
  if(leftover_entries==1){
    errno=blockbitmap_release_block(inode_parent.direct[block_index]);
    if(errno!=0){
      return -errno;
    }
    inode_parent.blocks=inode_parent.blocks-1;
  }
  struct dirent del_block[SWATFS_BSIZE / sizeof(struct dirent)];
  struct dirent last_block[SWATFS_BSIZE / sizeof(struct dirent)];

  if(last_dirent!=del_dirent){
    errno=disk_read_block(inode_parent.direct[block_del],del_block);
    if(errno!=0){
      return -errno;
    }
    errno=disk_read_block(inode_parent.direct[block_last],last_block);
    if(errno!=0){
      return -errno;
    }
    del_block[index_del].inum=last_block[index_last].inum;
    strcpy(del_block[index_del].name,last_block[index_last].name);
    errno=disk_write_block(inode_parent.direct[block_del],del_block);
    if(errno!=0){
      return -errno;
    }
  }
  inode_parent.size=inode_parent.size-sizeof(struct dirent);
  clock_gettime(CLOCK_REALTIME, &inode_parent.mtime);
  errno=inode_write(inum_parent,&inode_parent);
  if(errno!=0){
    return errno;
  }
  return 0;
}
//
//
//  Cases 1) removing block in middle, and final is in another block (things are fine here)
// 	     2) if removing element from a one-element block
// 		- need to decrease, inode.size, blocks, free block (release blockbitmap using bnum (inode.direct[..]))

/* Remove a directory from the file system.  The directory to remove must
 * be empty, or else you should return ENOTEMPTY. */
static int swatfs_rmdir(const char *path) {
    fprintf(stderr, "DEBUG: swatfs_rmdir, path: %s\n", path);
    struct inode inode;
    uint32_t inum;
    errno=directory_lookup(path,&inode,&inum);
    if(errno!=0){
      return errno;
    }
    if(inode.size>0){
      return -ENOTEMPTY;
    }
    return remover(path);
}

/* NOTE: rmdir() and unlink() are VERY similar in what they do.  I would
 *       strongly suggest using a helper function for these.  In my
 *       implementation, they were entirely identical except that rmdir() can
 *       return ENOTEMPTY if you try to remove a non-empty directory. */

/* Remove a regular file from the file system. */
static int swatfs_unlink(const char *path) {
    fprintf(stderr, "DEBUG: swatfs_unlink, path: %s\n", path);
    return remover(path);

}

/* Ensure a file is written out to disk.  You don't need to change this. */
static int swatfs_fsync(const char *path, int unused, struct fuse_file_info *fi) {
    /* Just sync the whole FS, we're not exactly a high performance FS. */
    if (disk_sync()) {
        return -errno;
    }
    return 0;
}

/* Ensure a directory is written out to disk.  You don't need to change this. */
static int swatfs_fsyncdir(const char *path, int unused, struct fuse_file_info *fi) {
    /* Just sync the whole FS, we're not exactly a high performance FS. */
    if (disk_sync()) {
        return -errno;
    }
    return 0;
}

static struct fuse_operations operations = {
    .destroy  = swatfs_destroy,
    .create   = swatfs_create,
    .fsync    = swatfs_fsync,
    .fsyncdir = swatfs_fsyncdir,
    .getattr  = swatfs_getattr,
    .mkdir    = swatfs_mkdir,
    .read     = swatfs_read,
    .readdir  = swatfs_readdir,
    .rmdir    = swatfs_rmdir,
    .truncate = swatfs_truncate,
    .unlink   = swatfs_unlink,
    .write    = swatfs_write,
};

/* Read and initialize the disk.  This mostly defers to fuse's main.
 * You shouldn't need to change this. */
int main(int argc, char **argv) {

    if (argc != 7) {
        fprintf(stderr, "Usage: %s -d -s <mount point> <disk image>\n", argv[0]);
        return 1;
    }

    /* Initialize the disk and free block map. */
    if (disk_open(argv[argc - 1])) {
        fprintf(stderr, "Could not open disk image:\n");
        perror("  disk_open");
        exit(1);
    }
    if (blockbitmap_cache_init()) {
        fprintf(stderr, "Failed to initialize block map cache:\n");
        perror("  blockbitmap_cache_init");
        exit(1);
    }

    argv[argc - 1] = NULL;
    argc -= 1;

    /* Let fuse do its thing. */
    return fuse_main(argc, argv, &operations, NULL);
}
