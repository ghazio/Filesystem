/* Swarthmore College, CS 45, Lab 5
 * Copyright (c) 2019 Swarthmore College Computer Science Department,
 * Swarthmore PA
 * Professor Kevin Webb */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include "swatfs_blockbitmap.h"
#include "swatfs_directory.h"
#include "swatfs_disk.h"
#include "swatfs_inode.h"

/* Notes about getting started: */

/* The purpose of directory_lookup is to walk through the directory tree,
reading through all the directories along a path until you find the
file/directory at the end.  It fills in the "result" inode and "inum" inode
number pointers, making those available to the caller.

Let's say you're given a path of "/dir1/dir2/filename".

To start with, you will always have to begin by looking in the root directory,
whose metadata lives at a known location: inode 0.

That is, if you read inode 0, the type of that inode's file (according to the
mode value) must be directory.  ANY TIME the mode indicates directory it means
that the data for this directory file is a series of zero or more struct dirent
values (directory entries) that associate a name with an inode number.

You're going to need to chop your path into tokens by splitting the string on
'/' characters.  Because the number of tokens is variable, you'll need to use a
loop to help you look through the directory at each step of the way.

To start with, you're looking for the first token ("dir1") in the first
directory (the root, whose inode is 0).  Once you find that, look for "dir2" in
the /dir1 directory, then look for "filename" in the /dir1/dir2 directory.  At
that point, you'll have run out of tokens, so the loop will terminate, and the
final result is the inode / inode number for the "filename" entry in
/dir1/dir2/.

So, how  do you read the entries in the directory to A) see whether or not the
name exists, and B) if if it does, get the inode number from it?

This is where you'll need to start looking at the file contents - the data
blocks of the file.  In this case, we'll be looking at the data contents of
files of type directory.  That's the important distinction we made in class
yesterday about file types to the OS.  If we see that a file is of type
directory, we know that we can interpret the file's data as a collection of
directory entries (struct dirents), which map names to inode numbers.

So, once you have the current inode ready to go, you'll need to access its data
blocks to get at the directory entries.

You may want to make this portion a separate helper function: a function takes
a directory inode and a token and scans through the directory entries and
returns the corresponding inode for that token, if it finds it (or error value
if it doesn't).

In this function, you would need to iterate through the directory's data.  How
much data does it have?  The inode tells you both the number of blocks and the
total size in bytes.  That's enough information to determine how many blocks
you need to look through, and for the last block, how much of that block to
look through.

For example, suppose you have a directory with 19 entries.  With the default
values in swatfs_types.h, you can fit 16 dirent structs in a block, so this
directory would have two blocks and a total size of (19 * 256 = 4864 bytes).
Since there are two blocks, the first one must be entirely full (4096 bytes),
so it's only the second block that is partially full.  If you do 4864 % 4096,
you'll see that there are 768 bytes (3 entries) left in the partial block.

To get at these entries, you'll need to do disk_read()'s.  Which block do you
need to read?  The one in the block map, which is the inode's direct array.
So, to get the first block, you'd do a disk_read with a block number of
inode.direct[0].  To get the next block, you'd use number inode.direct[1], and
so on.

When you read the block, you KNOW, for the reasons described above, that the
data will be dirent structs, so you can go ahead and type the data that way for
convenience.  The tips section of the lab page talks about this, but it would
look like:

struct dirent block[SWATFS_BSIZE / sizeof(struct dirent)];

This says "give me enough data to pack in as many dirent structs as I can into
one data block.

Putting it all together, you'd do a disk_read_block(inode.direct[0], block).
From there, you now have an array of dirent structs that you can iterate over,
checking each one's name field against the token you're looking for.  If
you find a match, you now have the inode for the next directory in the
path. */



/*helper function that takes in a directory inode and a token and scans through
 the directory entries and returns the corresponding inode for that token,
if it finds it (or error value if it doesn't).*/
uint32_t find_inodeNum(char* token, struct inode* dir_inode){

  int entry_size = sizeof(struct dirent);
  int max_entries = 16;

  int leftover_entries = (dir_inode->size % SWATFS_BSIZE)/entry_size;
  if(leftover_entries == 0 && dir_inode->size != 0){
    leftover_entries = 16;
  }
  struct dirent block[SWATFS_BSIZE / sizeof(struct dirent)];
//for each block in our dir_inode
printf("leftover_entries: %d\n",leftover_entries);
  for (int i = 0; i < dir_inode->blocks; i++){
    //diskread each dirent struct into a block
    errno=disk_read_block(dir_inode->direct[i],&block);
    if(errno!=0){
      return -errno;
    }
    //if the block isn't the last block (ie, it is full)
    if(i < dir_inode->blocks-1){
      //iterate through each entry
      for (int j =0; j < max_entries; j++){
        //check if dirent struct name matches the token
          printf("name%s and token:%s inode_num:%d \n", block[j].name,token, block[j].inum);
        if(strcmp(block[j].name,token)==0){

          //return inode name
          return block[j].inum;
        }
      }

      //if it is the last block (ie. not full of entries)
    }else{
      //iterate through the entries
        for (int j =0; j < leftover_entries; j++){
            printf("name%s and token:%s inode_num:%d \n", block[j].name,token, block[j].inum);
          //check if dirent struct name matches the token
          if(strcmp(block[j].name,token)==0){
           //return inode name
            return block[j].inum;
          }
        }
      }
  }
  //no such file or directory.
  return -ENOENT;
}



/* This function should return 0 on success.  On error, it should set errno and
 * return a negated value of errno.  In particular, it needs to set errno to
 * ENOENT when asked for a path that does not exist.
 * It also needs to fill in the resulting inode and inode number.*/
int directory_lookup(const char *path, struct inode *result, uint32_t *inum) {
    fprintf(stderr, "DEBUG: looking up path: %s\n", path);


    // Start at inode 0, which you can assume contains the metadata for
    // the root (/) directory.
    uint32_t inode_num = 0;
    struct inode dir_inode;
    errno = inode_read(inode_num, &dir_inode);
    if(!S_ISDIR(dir_inode.mode)){
      return -ENOTDIR;
    }
    //special case
    // TODO: Hack up the path string on the /'s.  If you plan to use strtok,
    // make sure you make a copy of the string first and hack up that.
    //copy the path variable
    char* path_copy = strdup(path);
    //read in the first token(tokenise by ':'' character)
    char* token = strtok(path_copy, "/");

    //while we dont run out of tokens
     while(token!=NULL){

       //reads in an inode and fills it in dir_inode based on inode number
       errno = inode_read(inode_num, &dir_inode);
       if(!S_ISDIR(dir_inode.mode)){
         return -ENOTDIR;
       }
       //error checking
       if(errno != 0){
         return -errno;
       }
       //look for the inode number of this token
       //if this inode number is inside the second directory, go into that
       inode_num = find_inodeNum(token, &dir_inode);
       if(inode_num == -ENOENT){
         printf("inside if errno statment\n");
         errno = ENOENT;
         return -errno;
       }
       //So, once you have the current inode ready to go, you'll need to access its data
      // blocks to get at the directory entries.

       //move to the next token
       token = strtok(NULL, "/");
     }
     // Once you have the inode number of the path's final destination,
     // use inode_read to fill in result and set *inum.
     *inum = inode_num;
     printf("inum: %d\n",*inum );

     errno = inode_read(*inum, result);

     //error checking
     if(errno != 0){
       return -errno;
     }


    return 0;
}
