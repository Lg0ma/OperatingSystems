/*
  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Copyright 2022-24

  University of Texas at El Paso, Department of Computer Science.

  Contributors: Christoph Lauter 
                Luis Gomez and 
                Ivan Armenta

  and based on 

  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

*/

/*
// The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.

   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.

   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.

   CAUTION:

   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.

   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.

   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.

   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.

   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!

   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:

   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.

   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.

   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.

   * And of course: your code MUST NOT SEGFAULT!

   It is reasonable to proceed in the following order:

   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinit_flag at mount
         time if you init_flag it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 

   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.

   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).

   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.

   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.

   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with "touch foo"
         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.

   (7)   Design and implement __myfs_mkdir_implem. Test as above.

   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:

         truncate -s 1024 foo

   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.

   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).

   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.

   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:

         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo

         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.

   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.

   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.

   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.

   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.

   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).

   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.
*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define BLOCK_SIZE 4096
#define NODE_SIZE 128
#define MAX_FILENAME 255
#define MAX_NODES 1024
#define ROOT_NODE 0
#define MAX_DATA_BLOCKS 2528 

//Info block for the filesystem
typedef struct{
    //flag to verify if already initialized
    uint8_t init_flag;
    //size of the filesystem
    size_t size;
    //offset to root node
    size_t root_node;
    //offset to node bitmap
    size_t free_node_bitmap; 
    //offset to data block bitmap
    size_t free_block_bitmap;
    //offset to node table
    size_t node_table;
    //offset to data blocks
    size_t data_blocks;
    //maximum number of data blocks
    size_t max_data_blocks;
}fs_info_block;

//Node in the filesystem 
typedef struct{
    //file type and permissions
    mode_t mode;
    //user id
    uid_t uid;
    //group id
    gid_t gid;  
    //size of file
    size_t size;
    //last access time
    time_t acc_time; 
    //last modification time
    time_t mod_time;
    //last change time
    time_t chng_time; 
    //offset to data block
    size_t data_block;
}node;

//Entry inside a Directory
typedef struct{
    //name of file
    char name[MAX_FILENAME + 1];
    //offset to node of file
    size_t node_offset;
}directory_entry;

// Function to compare two strings                   
int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

//Convert an offset to a pointer in the file system
static void* convert_off_to_ptr(void *fsptr, size_t fssize, size_t offset){
    if (offset >= fssize) {
        return NULL;
    } else {
        return (char *)fsptr + offset;
    }
}

//Initialize the root directory for the file system
static int initialize_filesystem(void *fsptr, size_t fssize){
    //Info block at beggining of file system
    fs_info_block *info_block = (fs_info_block*)fsptr;

    //File System already initialized 
    if(info_block->init_flag) {
        return 1;
    }
    
    //Initialize info block of file system
    info_block->init_flag = 1;
    info_block->size = fssize;
    info_block->root_node = sizeof(fs_info_block);
    info_block->free_node_bitmap = info_block->root_node + NODE_SIZE;
    info_block->free_block_bitmap = info_block->free_node_bitmap + (MAX_NODES / 8);
    info_block->node_table = info_block->free_block_bitmap + (MAX_DATA_BLOCKS / 8);
    info_block->data_blocks = info_block->node_table + (MAX_NODES * NODE_SIZE);
    info_block->max_data_blocks = MAX_DATA_BLOCKS; 

    //Initialize root node
    node *root = (node*)convert_off_to_ptr(fsptr, fssize, info_block->root_node);
    root->mode = S_IFDIR | 0755;
    root->uid = getuid();
    root->gid = getgid();
    root->size = 0;
    root->acc_time = root->mod_time = root->chng_time = time(NULL);
    root->data_block = info_block->data_blocks;

    //Initialize root directory
    directory_entry *root_dir = (directory_entry*)convert_off_to_ptr(fsptr, fssize, root->data_block);
    //Add .
    strcpy(root_dir[0].name, ".");
    root_dir[0].node_offset = info_block->root_node;
    //Add ..
    strcpy(root_dir[1].name, "..");
    root_dir[1].node_offset = info_block->root_node;
    root->size += 2 * sizeof(directory_entry);

    //Initialize node bitmap
    memset(convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap), 0, MAX_NODES / 8);

    //Mark root as used
    uint8_t *node_bitmap = (uint8_t *)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
    node_bitmap[0] |= (uint8_t)1;

    //Initialize data block bitmap
    memset(convert_off_to_ptr(fsptr, fssize, info_block->free_block_bitmap), 0, MAX_DATA_BLOCKS / 8);

    //Mark root's data block as used
    uint8_t *data_bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
    if (data_bitmap) data_bitmap[0] |= 1;

    // Successful initialization
    return 1;
}


//Retrieve a node in the filesystem
static node* get_node(void *fsptr, size_t fssize, const char * path, size_t *node_convert_off_to_ptr){
    //Get initial info from the filesystem
    fs_info_block *info_block = (fs_info_block*)fsptr;
    node *curr_node = (node *)convert_off_to_ptr(fsptr, fssize, info_block->root_node);
    size_t curr_offset = info_block->root_node;

    //Path is root
    if(!my_strcmp
    (path, "/")){
        *node_convert_off_to_ptr = curr_offset;
        return curr_node;
    }

    //Tokenize path
    char *copy_path = strdup(path);

    //No path provided
    if (!copy_path) return NULL;

    //Tokenize path
    char *token = strtok(copy_path, "/");
    while(token){
        if(!(curr_node->mode & S_IFDIR)){
                free(copy_path);
                return NULL;
        }

        //Iterate through directory
        directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, curr_node->data_block);
        size_t num_entries = curr_node->size / sizeof(directory_entry), next_offset = 0;
        node *next_node = NULL;
        int node_found = 0;

        for(size_t i = 0; i < num_entries; i++){
                if(!my_strcmp
                (entries[i].name, token)){
                    next_offset = entries[i].node_offset;
                    next_node = (node *)convert_off_to_ptr(fsptr, fssize, next_offset);
                    node_found = 1;
                    break;
                }
        }

        //Node not found
        if(!node_found){
                free(copy_path);
                return NULL;
        }

        //Move to next node
        curr_node = next_node;
        curr_offset = next_offset;
        token = strtok(NULL, "/");
    }

    //Cleanup and return
    free(copy_path);
    if(node_convert_off_to_ptr) *node_convert_off_to_ptr = curr_offset;
    return curr_node;
}


// Split path into parent directory and base name
static int split_path(const char *path, char **parent_path, char **base_name) {
    // Directory is empty
    if (!path || !parent_path || !base_name) return -1;

    // Create path copy
    char *copy_path = strdup(path);
    if (!copy_path) return -1;

    //Find last slash
    char *last_slash = strrchr(copy_path, '/');
    // Invalid path 
    if (last_slash == NULL) {
        free(copy_path);
        return -1;
    }


    //Parent directory is root
    if (last_slash == copy_path) *parent_path = strdup("/");
    else {
        *last_slash = '\0';
        *parent_path = strdup(copy_path);
    }
    if(!(*parent_path)) {
        free(copy_path);
        return -1;
    }

    //base name
    *base_name = strdup(last_slash + 1);
    if (!(*base_name)) {
        free(copy_path);
        free(*parent_path);
        return -1;
    }

    //Cleanup and return
    free(copy_path);
    return 0;
}


//Find available node
static size_t find_available_node(void *fsptr, size_t fssize) {
    size_t node_num, node_offset;
    //Make info block
    fs_info_block *info_block = (fs_info_block*)fsptr;
    //Get offset to pointer for start of bitmap
    uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
    if (!bitmap) return (size_t)-1;

    //Iterate through nodes
    for (size_t byte = 0; byte < MAX_NODES / 8; byte++) {
        //Byte not equal to -1 char
        if (bitmap[byte] != 0xFF) {
            //Iterate through bits
            for (int bit = 0; bit < 8; bit++) {
                node_num = byte * 8 + bit;
                //Reached last node
                if (node_num >= MAX_NODES) break;
                //Check if current node, is free
                if (!(bitmap[byte] & (1 << bit))) {
                    //Mark node, as used
                    bitmap[byte] |= (1 << bit);
                    //Calculate offset to node
                    node_offset = info_block->node_table + node_num * NODE_SIZE;
                    //Return offset
                    return node_offset;
                }
            }
        }
    }

    return (size_t)-1;
}


int new_dir_entry(void *fsptr, size_t fssize, node *dir_node, size_t dir_node_offset, const char *name, size_t new_node_offset) {
    //Get current number of entries from dir
    size_t num_entries = dir_node->size / sizeof(directory_entry), max_entries = BLOCK_SIZE / sizeof(directory_entry);

    //We can't add more directory entries
    if (num_entries >= max_entries) return -1; 

    //Make new entry
    directory_entry *new_entry = (directory_entry*)convert_off_to_ptr(fsptr, fssize, dir_node->data_block + num_entries * sizeof(directory_entry));

    //Bad offset
    if (!new_entry) return -1;

    //Copy name into new directory entry
    strncpy(new_entry->name, name, MAX_FILENAME - 1);
    new_entry->name[MAX_FILENAME - 1] = '\0'; 

    //Set offset (huh, kinda rhymed)
    new_entry->node_offset = new_node_offset;

    //Update size of dir
    dir_node->size += sizeof(directory_entry);

    //Update times
    dir_node->mod_time = dir_node->chng_time = time(NULL);

    return 0; 
}


//Remove entry from directory
static int rem_dir_entry(void *fsptr, size_t fssize, node *dir_node, size_t dir_node_offset, const char *name) {
    //Get entries
    directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, dir_node->data_block);
    if (!entries) return -1;

    //Gets number of entries used for directory
    size_t num_entries = dir_node->size / sizeof(directory_entry);
    size_t target_index = num_entries;

    // Try to find it
    for (size_t i = 0; i < num_entries; i++) {
        if (my_strcmp
        (entries[i].name, name) == 0) {
                target_index = i;
                break;
        }
    }

    //Entry not found
    if (target_index == num_entries) return -1;
    
    for (size_t i = target_index; i < num_entries - 1; i++) entries[i] = entries[i + 1];

    //Zero out the last entry (DEstroy)
    memset(&entries[num_entries - 1], 0, sizeof(directory_entry));

    // Update the size
    dir_node->size -= sizeof(directory_entry);

    //Update the times
    dir_node->mod_time = dir_node->chng_time = time(NULL);

    return 0; 
}

// Find availale data block
static size_t find_free_data_block(void *fsptr, size_t fssize) {
    //Get the info
    fs_info_block *info_block = (fs_info_block*)fsptr;
    //Get the max number of data blocks
    size_t max_data_blocks = info_block->max_data_blocks; 
    //Get the bitmap offset
    unsigned char *bitmap = (unsigned char*)convert_off_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
    if (!bitmap) return (size_t)-1;

    //Block offset, 'init
    size_t block_offset;

    //Iterate through data blocks
    for (size_t block_num = 0; block_num < max_data_blocks; block_num++) {
        //Check if block is free
        if (!(bitmap[block_num / 8] & (1 << (block_num % 8)))) {
            //Mark block as used
            bitmap[block_num / 8] |= (1 << (block_num % 8));
            //Calculate block offset
            block_offset = info_block->data_blocks + block_num * BLOCK_SIZE;
            return block_offset;
        }
    }
    //No free data blocks
    return (size_t)-1;
}

//Frees data block and updates bitmap 
static int free_data_block(void *fsptr, size_t fssize, size_t block_offset) {
    //Get the info block
    fs_info_block *info_block = (fs_info_block*)fsptr;
    //Check if block offset is valid
    if (block_offset < info_block->data_blocks || block_offset >= fssize) return -1;

    //Calculate block number
    size_t block_num = (block_offset - info_block->data_blocks) / BLOCK_SIZE;
    if (block_num >= info_block->max_data_blocks) return -1;

    //Get the bitmap
    uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
    if (!bitmap) return -1;

    //Free the block
    bitmap[block_num / 8] &= ~(1 << (block_num % 8)); 
    return 0; 
}

// Get node bitmap for data block
uint8_t *get_bitmap(void *fsptr, size_t fssize) {
    // Ensure that the fs_info_block is valid and accessible
    if (!fsptr || fssize == 0) {
        return NULL;
    }

    // Cast the input pointer to fs_info_block structure
    fs_info_block *info_block = (fs_info_block*)fsptr;

    // Ensure that the free_block_bitmap field is valid
    if (info_block->free_block_bitmap >= fssize) {
        return NULL;  // If the bitmap offset is out of bounds, return NULL
    }

    // Get the offset of the free block bitmap and return the pointer to it
    return (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
}

// Get number of free blocks
size_t calculate_free_blocks(void *fsptr, size_t fssize) {
    // Get the bitmap pointer
    unsigned char *bitmap = get_bitmap(fsptr, fssize);
    if (!bitmap) return 0;

    // Calculate number of blocks in the bitmap
    size_t num_bitmap_blocks = fssize / BLOCK_SIZE;  // Number of blocks represented in the bitmap
    size_t free_blocks = 0;

    // Iterate through the bitmap and count the free blocks
    for (size_t i = 0; i < num_bitmap_blocks; i++) {
        // For each byte in the bitmap, check each of the 8 bits
        for (int j = 0; j < 8; j++) {
            // If the bit is 0, the block is free
            if (!(bitmap[i] & (1 << j))) {
                free_blocks++;
            }
        }
    }

    // Return the number of free blocks
    return free_blocks;
}

// Implements an emulation of the stat system call on the filesystem 
//    of size fssize pointed to by fsptr. 

//    If path can be followed and describes a file or directory 
//    that exists and is accessable, the access information is 
//    put into stbuf. 

//    On success, 0 is returned. On failure, -1 is returned and 
//    the appropriate error code is put into *errnoptr.

//    man 2 stat documents all possible error codes and gives more detail
//    on what fields of stbuf need to be filled in. Essentially, only the
//    following fields need to be supported:

//    st_uid      the value passed in argument
//    st_gid      the value passed in argument
//    st_mode     (as fixed values S_IFDIR | 0755 for directories,
//                                 S_IFREG | 0755 for files)
//    st_nlink    (as many as there are subdirectories (not files) for directories
//                 (including . and ..),
//                 1 for files)
//    st_size     (supported only for files, where it is the real file size)
//    st_atim
//    st_mtim

int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr, uid_t uid, gid_t gid, const char *path, struct stat *stbuf) {
    //Check that filesystem was initialized sucessfully
    if(!initialize_filesystem(fsptr, fssize)){
        *errnoptr = EFAULT;
        return -1;
    }

    //Find node to path
    size_t node_offset;
    node *node = get_node(fsptr, fssize, path, &node_offset);
    if(!node){
        *errnoptr = ENOENT;
        return -1;
    }

    //Populate stbuf
    memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_uid = node->uid;
    stbuf->st_gid = node->gid;
    stbuf->st_mode = node->mode;
    stbuf->st_size = node->size;
    stbuf->st_atime = node->acc_time;
    stbuf->st_mtime = node->mod_time;
    stbuf->st_ctime = node->chng_time;

    //Set num links for directories
    if(node->mode & S_IFDIR){
        size_t num_entries = node->size / sizeof(directory_entry);
        stbuf->st_nlink = num_entries;
    //Set 1 link for a file
    }else if(node->mode & S_IFREG){
        stbuf->st_nlink = 1;
    //Ooops, wrong type of fuiel, thoug, can we do links?
    }else{
        *errnoptr = EINVAL;
        return -1;
    }

    return 0;
}

// Implements an emulation of the readdir system call on the filesystem 
//    of size fssize pointed to by fsptr. 

//    If path can be followed and describes a directory that exists and
//    is accessable, the names of the subdirectories and files 
//    contained in that directory are output into *namesptr. The . and ..
//    directories must not be included in that listing.

//    If it needs to output file and subdirectory names, the function
//    starts by allocating (with calloc) an array of pointers to
//    characters of the right size (n entries for n names). Sets
//    *namesptr to that pointer. It then goes over all entries
//    in that array and allocates, for each of them an array of
//    characters of the right size (to hold the i-th name, together 
//    with the appropriate '\0' terminator). It puts the pointer
//    into that i-th array entry and fills the allocated array
//    of characters with the appropriate name. The calling function
//    will call free on each of the entries of *namesptr and 
//    on *namesptr.

//    The function returns the number of names that have been 
//    put into namesptr. 

//    If no name needs to be reported because the directory does
//    not contain any file or subdirectory besides . and .., 0 is 
//    returned and no allocation takes place.

//    On failure, -1 is returned and the *errnoptr is set to 
//    the appropriate error code. 

//    The error codes are documented in man 2 readdir.

//    In the case memory allocation with malloc/calloc fails, failure is
//    indicated by returning -1 and setting *errnoptr to EINVAL.__myfs_readdir_implem

int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,  const char *path, char ***namesptr) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Find node for map
    size_t node_offset;
    node *dir_node = get_node(fsptr, fssize, path, &node_offset);
    if (!dir_node) {
        *errnoptr = ENOENT;
        return -1;
    }

    //If NODE not dir
    if (!(dir_node->mode & S_IFDIR)) {
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Get directory entries
    directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, dir_node->data_block);
    if (!entries) {
        *errnoptr = EIO;
        return -1;
    }

    //Get number of entries
    size_t num_entries = dir_node->size / sizeof(directory_entry);
    size_t valid_entries = 0;

    //Count entries except .. .
    for (size_t i = 0; i < num_entries; i++) if (my_strcmp
    (entries[i].name, ".") && my_strcmp
    (entries[i].name, "..")) valid_entries++;

    //Ret 0 if no valid entries
    if (!valid_entries) {
        *namesptr = NULL; 
        return 0;
    }

    //Allocate names array
    char **names_array = calloc(valid_entries, sizeof(char *));
    if (!names_array) {
        *errnoptr = EINVAL;
        return -1;
    }

    size_t current_name = 0;

    //Copy each name
    for (size_t i = 0; i < num_entries; i++) {
        //Skip . and ..
        if (!my_strcmp
        (entries[i].name, ".") || !my_strcmp
        (entries[i].name, "..")) continue;

        //Allocate memory for str
        size_t name_len = strlen(entries[i].name);
        names_array[current_name] = malloc(name_len + 1);
        if (!names_array[current_name]) {
            //Malloc failed, clean
            for (size_t j = 0; j < current_name; j++) free(names_array[j]);
            free(names_array);
            *errnoptr = EINVAL;
            return -1;
        }

        //Copy name
        strcpy(names_array[current_name], entries[i].name);
        current_name++;
    }

    //Assign array to namesptr
    *namesptr = names_array;

    //Return num names
    return valid_entries;
}

// Implements an emulation of the mknod system call for regular files
//    on the filesystem of size fssize pointed to by fsptr.

//    This function is called only for the creation of regular files.

//    If a file gets created, it is of size zero and has default
//    ownership and mode bits.

//    The call creates the file indicated by path.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 mknod.


int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Split path
    char *parent_path = NULL;
    char *file_name = NULL;
    if (split_path(path, &parent_path, &file_name)) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Find parent dir
    size_t parent_node_offset;
    node *parent_dir = get_node(fsptr, fssize, parent_path, &parent_node_offset);
    if (!parent_dir) {
        free(parent_path);
        free(file_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Verify parent is dir
    if (!(parent_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(file_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Check if file exists
    directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, parent_dir->data_block);
    if (!entries) {
        free(parent_path);
        free(file_name);
        *errnoptr = EIO;
        return -1;
    }

    size_t num_entries = parent_dir->size / sizeof(directory_entry);
    for (size_t i = 0; i < num_entries; i++) {
        if (!my_strcmp
        (entries[i].name, file_name)) {
                free(parent_path);
                free(file_name);
                *errnoptr = EEXIST;
                return -1;
        }
    }

    //Find free node
    size_t new_node_offset = find_available_node(fsptr, fssize);
    if (new_node_offset == (size_t)-1) {
        free(parent_path);
        free(file_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    //Initialize node
    node *new_node = (node *)convert_off_to_ptr(fsptr, fssize, new_node_offset);
    if (!new_node) {
        //Unmark the node in bitmap
        fs_info_block *info_block = (fs_info_block*)fsptr;
        unsigned char *bitmap = (unsigned char*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
        size_t node_num = (new_node_offset - info_block->node_table) / NODE_SIZE;
        if (node_num < MAX_NODES) bitmap[node_num / 8] &= ~(1 << (node_num % 8));
        free(parent_path);
        free(file_name);
        *errnoptr = EIO;
        return -1;
    }

    //Set node info for file
    new_node->mode = S_IFREG | 0644;
    new_node->uid = getuid();
    new_node->gid = getgid();
    new_node->size = 0;
    new_node->acc_time = new_node->mod_time = new_node->chng_time = time(NULL);
    new_node->data_block = 0;

    //Add entry to parent dir
    if (new_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, file_name, new_node_offset) != 0) {
        // Failed to add dir, unmark the node in bitmap
        fs_info_block *info_block = (fs_info_block*)fsptr;
        uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
        size_t node_num = (new_node_offset - info_block->node_table) / NODE_SIZE;
        if (node_num < MAX_NODES) bitmap[node_num / 8] &= ~(1 << (node_num % 8));
        //Reset node
        memset(new_node, 0, sizeof(node));
        free(parent_path);
        free(file_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    //Clean up
    free(parent_path);
    free(file_name);

    return 0;
}


        
// Implements an emulation of the unlink system call for regular files
//    on the filesystem of size fssize pointed to by fsptr.

//    This function is called only for the deletion of regular files.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 unlink.


int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Split path into parent directory and file name
    char *parent_path = NULL;
    char *file_name = NULL;
    if (split_path(path, &parent_path, &file_name)) {
        *errnoptr = EINVAL; 
        return -1;
    }

    //Find parent dir
    size_t parent_node_offset;
    node *parent_dir = get_node(fsptr, fssize, parent_path, &parent_node_offset);
    if (!parent_dir) {
        free(parent_path);
        free(file_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Verify parent is dir
    if (!(parent_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(file_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Get entries
    directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, parent_dir->data_block);
    if (!entries) {
        free(parent_path);
        free(file_name);
        *errnoptr = EIO;
        return -1;
    }

    //Get number of entries
    size_t num_entries = parent_dir->size / sizeof(directory_entry);
    size_t target_index = num_entries;

    //Seek target
    for (size_t i = 0; i < num_entries; i++) {
        if (!my_strcmp
        (entries[i].name, file_name)) {
                target_index = i;
                break;
        }
    }

    //404, File not node_found
    if (target_index == num_entries) {
        free(parent_path);
        free(file_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Get offset
    size_t target_node_offset = entries[target_index].node_offset;
    node *target_node = (node *)convert_off_to_ptr(fsptr, fssize, target_node_offset);
    if (!target_node) {
        free(parent_path);
        free(file_name);
        *errnoptr = EIO;
        return -1;
    }

    //Verify it's a file
    if (!(target_node->mode & S_IFREG)) {
        free(parent_path);
        free(file_name);
        *errnoptr = EISDIR;
        return -1;
    }

    //Remove directory entry
    if (rem_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, file_name)) {
        free(parent_path);
        free(file_name);
        *errnoptr = EIO;
        return -1;
    }

    //Free node in bitmap
    fs_info_block *info_block = (fs_info_block*)fsptr;
    //Git bitmap
    uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
    if (!bitmap) {
        *errnoptr = EIO; 
        return -1;
    }
    //Get node number
    size_t node_num = (target_node_offset - info_block->node_table) / NODE_SIZE;
    // Node don't exist
    if (node_num >= MAX_NODES) {
        *errnoptr = EIO;
        return -1;
    }

    //Unmark node
    bitmap[node_num / 8] &= ~(1 << (node_num % 8));

    //Free data block allocated
    if (target_node->data_block) {
        if (free_data_block(fsptr, fssize, target_node->data_block) != 0) {
                *errnoptr = EIO; 
                return -1;
        }
    } 

    //Set NODE to 0
    memset(target_node, 0, sizeof(node));

    //Cleanup and ret
    free(parent_path);
    free(file_name);
    return 0;
}

// Implements an emulation of the rmdir system call on the filesystem 
//    of size fssize pointed to by fsptr. 

//    The call deletes the directory indicated by path.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The function call must fail when the directory indicated by path is
//    not empty (if there are files or subdirectories other than . and ..).

//    The error codes are documented in man 2 rmdir.


int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,const char *path) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Split path into parent directory and directory to rm
    char *parent_path = NULL;
    char *dir_name = NULL;
    if (split_path(path, &parent_path, &dir_name) != 0) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Find parent directory
    size_t parent_node_offset;
    node *parent_dir = get_node(fsptr, fssize, parent_path, &parent_node_offset);
    if (!parent_dir) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Check parent is directory
    if (!(parent_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Get target directory
    size_t target_node_offset;
    node *target_dir = get_node(fsptr, fssize, path, &target_node_offset);
    if (!target_dir) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Check if target directory is a directory
    if (!(target_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //directory must be empty
    size_t num_entries = target_dir->size / sizeof(directory_entry);
    if (num_entries > 2) { // More than . and ..
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTEMPTY;
        return -1;
    }

    //Check only entries are . and ..
    directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, target_dir->data_block);
    if (!entries) {
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    if (num_entries == 1) {
        //First entry must be .
        if (my_strcmp
        (entries[0].name, ".")) {
            free(parent_path);
            free(dir_name);
            *errnoptr = ENOTEMPTY;
            return -1;
        }
    } else if (num_entries == 2) {
        //Check if contains . ..
        int has_dot = 0, has_dotdot = 0;
        for (size_t i = 0; i < num_entries; i++) {
            if (my_strcmp
            (entries[i].name, ".") == 0) has_dot = 1;
            if (my_strcmp
            (entries[i].name, "..") == 0) has_dotdot = 1;
        }
        if (!has_dot || !has_dotdot) {
            free(parent_path);
            free(dir_name);
            *errnoptr = ENOTEMPTY;
            return -1;
        }
    }

    // If found directory is found remove it 
    if (rem_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, dir_name) != 0) {
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    //Free directory data blcok
    if (target_dir->data_block) {
        if (free_data_block(fsptr, fssize, target_dir->data_block)) {
            new_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, dir_name, target_node_offset);
            free(parent_path);
            free(dir_name);
            *errnoptr = EIO;
            return -1;
        }
    }

    //Free the target directory's node in the node bitmap
    fs_info_block *info_block = (fs_info_block*)fsptr;
    unsigned char *node_bitmap = (unsigned char*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
    if (!node_bitmap) {
        new_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, dir_name, target_node_offset);
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    //Get numbet of node
    size_t node_num = (target_node_offset - info_block->node_table) / NODE_SIZE;
    if (node_num >= MAX_NODES) {
        new_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, dir_name, target_node_offset);
        if (target_dir->data_block) free_data_block(fsptr, fssize, target_dir->data_block);
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    //Mark node as free
    node_bitmap[node_num / 8] &= ~(1 << (node_num % 8));

    //Set node to 0
    memset(target_dir, 0, sizeof(node));

    //Cleanup and ret
    free(parent_path);
    free(dir_name);
    return 0;
}

// Implements an emulation of the mkdir system call on the filesystem 
//    of size fssize pointed to by fsptr. 

//    The call creates the directory indicated by path.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 mkdir.


int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Split path into parent and child
    char *parent_path = NULL;
    char *dir_name = NULL;
    if (split_path(path, &parent_path, &dir_name) != 0) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Find parent node
    size_t parent_node_offset;
    node *parent_dir = get_node(fsptr, fssize, parent_path, &parent_node_offset);
    if (!parent_dir) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Check if parent is directory
    if (!(parent_dir->mode & S_IFDIR)) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Get entries
    directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, parent_dir->data_block);
    if (!entries) {
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    //Check if directory exists
    size_t num_entries = parent_dir->size / sizeof(directory_entry);
    for (size_t i = 0; i < num_entries; i++) {
        if (!my_strcmp
        (entries[i].name, dir_name)) {
            free(parent_path);
            free(dir_name);
            *errnoptr = EEXIST;
            return -1;
        }
    }

    //Find free node for new dir
    size_t new_node_offset = find_available_node(fsptr, fssize);
    if (new_node_offset == (size_t)-1) {
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    //New node
    node *new_dir_node = (node *)convert_off_to_ptr(fsptr, fssize, new_node_offset);
    if (!new_dir_node) {
        //Unmark node in bitmap
        fs_info_block *info_block = (fs_info_block*)fsptr;
        uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
        size_t node_num = (new_node_offset - info_block->node_table) / NODE_SIZE;
        if (node_num < MAX_NODES) bitmap[node_num / 8] &= ~(1 << (node_num % 8));
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    //Get data block for new directory
    size_t data_block_offset = find_free_data_block(fsptr, fssize);
    if (data_block_offset == (size_t)-1) {
        //Unmark node in bitmap
        fs_info_block *info_block = (fs_info_block*)fsptr;
        unsigned char *bitmap = (unsigned char*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
        size_t node_num = (new_node_offset - info_block->node_table) / NODE_SIZE;
        if (node_num < MAX_NODES) bitmap[node_num / 8] &= ~(1 << (node_num % 8));
        //Reset node
        memset(new_dir_node, 0, sizeof(node));
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    //Fill up new directory node
    new_dir_node->mode = S_IFDIR | 0755;
    new_dir_node->uid = getuid();
    new_dir_node->gid = getgid();
    new_dir_node->size = 0;
    new_dir_node->acc_time = new_dir_node->mod_time = new_dir_node->chng_time = time(NULL);
    new_dir_node->data_block = data_block_offset;

    //Initialize new entries(add "." and "..")
    directory_entry *new_dir_entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, data_block_offset);
    if (!new_dir_entries) {
        //Free data block at data_block_offset
        free_data_block(fsptr, fssize, data_block_offset);
        //Unmark node in bitmap
        fs_info_block *info_block = (fs_info_block*)fsptr;
        uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
        size_t node_num = (new_node_offset - info_block->node_table) / NODE_SIZE;
        if (node_num < MAX_NODES) bitmap[node_num / 8] &= ~(1 << (node_num % 8));
        // Reset the node 
        memset(new_dir_node, 0, sizeof(node));
        free(parent_path);
        free(dir_name);
        *errnoptr = EIO;
        return -1;
    }

    //Add .
    strcpy(new_dir_entries[0].name, ".");
    new_dir_entries[0].node_offset = new_node_offset;
    //Add ..
    strcpy(new_dir_entries[1].name, "..");
    new_dir_entries[1].node_offset = parent_node_offset;
    new_dir_node->size += 2 * sizeof(directory_entry);

    //Add new directory to parent
    if (new_dir_entry(fsptr, fssize, parent_dir, parent_node_offset, dir_name, new_node_offset)) {
        //Free data block
        free_data_block(fsptr, fssize, data_block_offset);
        //Unmark node in bitmap
        fs_info_block *info_block = (fs_info_block*)fsptr;
        uint8_t *bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_node_bitmap);
        size_t node_num = (new_node_offset - info_block->node_table) / NODE_SIZE;
        if (node_num < MAX_NODES) bitmap[node_num / 8] &= ~(1 << (node_num % 8));
        
        //Reset node
        memset(new_dir_node, 0, sizeof(node));
        free(parent_path);
        free(dir_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    //Cleanup and return
    free(parent_path);
    free(dir_name);

    return 0;
}

// Implements an emulation of the rename system call on the filesystem 
//    of size fssize pointed to by fsptr. 

//    The call moves the file or directory indicated by from to to.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    Caution: the function does more than what is hinted to by its name.
//    In cases the from and to paths differ, the file is moved out of 
//    the from path and added to the to path.

//    The error codes are documented in man 2 rename.


int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr, const char *from, const char *to) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Can't rename root directory
    if (!my_strcmp
    (from, "/")) {
        *errnoptr = EBUSY;
        return -1;
    }

    //Split from name into parent and child
    char *from_parent_path = NULL;
    char *from_base_name = NULL;
    if (split_path(from, &from_parent_path, &from_base_name)) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Split to name into parent and child
    char *to_parent_path = NULL;
    char *to_base_name = NULL;
    if (split_path(to, &to_parent_path, &to_base_name)) {
        free(from_parent_path);
        free(from_base_name);
        *errnoptr = EINVAL;
        return -1;
    }

    //Find from parent node
    size_t from_parent_node_offset;
    node *from_parent_dir = get_node(fsptr, fssize, from_parent_path, &from_parent_node_offset);
    if (!from_parent_dir) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Check from parent is accessable
    if (!(from_parent_dir->mode & S_IFDIR)) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Find target node to reanme
    size_t from_node_offset;
    node *from_node = get_node(fsptr, fssize, from, &from_node_offset);
    if (!from_node) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Find to parent directory node
    size_t to_parent_node_offset;
    node *to_parent_dir = get_node(fsptr, fssize, to_parent_path, &to_parent_node_offset);
    if (!to_parent_dir) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOENT;
        return -1;
    }

    //Check to parent is directory
    if (!(to_parent_dir->mode & S_IFDIR)) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOTDIR;
        return -1;
    }

    //Check to path's existance
    size_t to_node_offset;
    node *to_node = get_node(fsptr, fssize, to, &to_node_offset);
    if (to_node) {
        if (to_node->mode & S_IFDIR) {
            //To is directory and empty, and from is dir
            if (!(from_node->mode & S_IFDIR)) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                *errnoptr = EISDIR;
                return -1;
            }

            //Check if is empty  
            size_t to_num_entries = to_node->size / sizeof(directory_entry);
            if (to_num_entries > 2) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                *errnoptr = ENOTEMPTY;
                return -1;
            }

            //Verify only entries are . & ..
            directory_entry *to_entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, to_node->data_block);
            if (!to_entries) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                *errnoptr = EIO;
                return -1;
            }

            //At least contains .
            if (to_num_entries == 1) {
                if (my_strcmp
                (to_entries[0].name, ".") != 0) {
                    free(from_parent_path);
                    free(from_base_name);
                    free(to_parent_path);
                    free(to_base_name);
                    *errnoptr = ENOTEMPTY;
                    return -1;
                }
            } else if (to_num_entries == 2) {
                int has_dot = 0, has_dotdot = 0;
                for (size_t i = 0; i < to_num_entries; i++) {
                    if (my_strcmp
                    (to_entries[i].name, ".") == 0) has_dot = 1;
                    if (my_strcmp
                    (to_entries[i].name, "..") == 0) has_dotdot = 1;
                }
                if (!has_dot || !has_dotdot) {
                    free(from_parent_path);
                    free(from_base_name);
                    free(to_parent_path);
                    free(to_base_name);
                    *errnoptr = ENOTEMPTY;
                    return -1;
                }
            }

            //Remove to directory
            if (__myfs_rmdir_implem(fsptr, fssize, errnoptr, to)) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                return -1;
            }
        } else {
            // Not able to unlink node from "directory"
            if (__myfs_unlink_implem(fsptr, fssize, errnoptr, to)) {
                free(from_parent_path);
                free(from_base_name);
                free(to_parent_path);
                free(to_base_name);
                return -1;
            }
        }
    }

    // Remove directory entry from parent
    if (rem_dir_entry(fsptr, fssize, from_parent_dir, from_parent_node_offset, from_base_name)) {
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = EIO;
        return -1;
    }

    //Add directory entry to parent
    if (new_dir_entry(fsptr, fssize, to_parent_dir, to_parent_node_offset, to_base_name, from_node_offset)) {
        new_dir_entry(fsptr, fssize, from_parent_dir, from_parent_node_offset, from_base_name, from_node_offset);
        free(from_parent_path);
        free(from_base_name);
        free(to_parent_path);
        free(to_base_name);
        *errnoptr = ENOSPC;
        return -1;
    }

    //Cleanup
    free(from_parent_path);
    free(from_base_name);
    free(to_parent_path);
    free(to_base_name);
    return 0;
}

// Implements an emulation of the truncate system call on the filesystem 
//    of size fssize pointed to by fsptr. 

//    The call changes the size of the file indicated by path to offset
//    bytes.

//    When the file becomes smaller due to the call, the extending bytes are
//    removed. When it becomes larger, zeros are appended.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 truncate.


int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, off_t offset) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Offset is bad return -1
    if (offset < 0) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Find node for path
    size_t node_offset;
    node *file_node = get_node(fsptr, fssize, path, &node_offset);
    if (!file_node) {
        *errnoptr = ENOENT;
        return -1;//failed to find node
    }

    //No file return -1
    if (!(file_node->mode & S_IFREG)) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Truncate
    if (offset < file_node->size) {
        if (!offset) {
            //Truncate file to zero size by freeing data block
            if (file_node->data_block) {
                if (free_data_block(fsptr, fssize, file_node->data_block)) {
                    *errnoptr = EIO;
                    return -1;
                }
                file_node->data_block = 0;
            }
        } else {
            //Truncate existing data block
            if (!file_node->data_block) {
                *errnoptr = EIO;
                return -1;
            }

            //Check offset is < BLOCK_SIZE
            if ((size_t)offset > BLOCK_SIZE) {
                *errnoptr = EFBIG;
                return -1;
            }

            //Get offset to datablock
            void *data_ptr = convert_off_to_ptr(fsptr, fssize, file_node->data_block + offset);
            if (!data_ptr) {
                *errnoptr = EIO;
                return -1;
            }

            //Zero fill bytes
            size_t bytes_to_zero = file_node->size - offset;
            memset(data_ptr, 0, bytes_to_zero);
        }

        //Update node size
        file_node->size = offset;
        //Time to update the time
        file_node->mod_time = file_node->chng_time = time(NULL);

        return 0;
    }

    //Truncate to bigger size
    if (offset > file_node->size) {
        //File is too big return -1
        if ((size_t)offset > BLOCK_SIZE) {
            *errnoptr = EFBIG;
            return -1;
        }

        //Allocate data block memory
        if (!file_node->data_block) {
            size_t data_block_offset = find_free_data_block(fsptr, fssize);
            if (data_block_offset == (size_t)-1) {
                *errnoptr = ENOSPC;
                return -1;
            }

            //Mark the data block as used
            fs_info_block *info_block = (fs_info_block*)fsptr;
            uint8_t *data_bitmap = (uint8_t*)convert_off_to_ptr(fsptr, fssize, info_block->free_block_bitmap);
            if (data_bitmap) {
                size_t block_num = (data_block_offset - info_block->data_blocks) / BLOCK_SIZE;
                if (block_num >= MAX_DATA_BLOCKS) {
                    free_data_block(fsptr, fssize, data_block_offset);
                    *errnoptr = EIO;
                    return -1;
                }
                data_bitmap[block_num / 8] |= (1 << (block_num % 8));
            } else {
                //Unable to access data block bitmap
                free_data_block(fsptr, fssize, data_block_offset);
                *errnoptr = EIO;
                return -1;
            }

            //Initialize the data block
            void *data_ptr = convert_off_to_ptr(fsptr, fssize, data_block_offset);
            if (!data_ptr) {
                free_data_block(fsptr, fssize, data_block_offset);
                *errnoptr = EIO;
                return -1;
            }

            //Zero fill the block
            memset(data_ptr, 0, BLOCK_SIZE);

            //Assign data block to node
            file_node->data_block = data_block_offset;
        }

        //Get pointer to data block
        void *data_ptr = convert_off_to_ptr(fsptr, fssize, file_node->data_block);
        if (!data_ptr) {
            *errnoptr = EIO;
            return -1;
        }

        //Get num bytes to 0 fill
        size_t bytes_to_zero = offset - file_node->size;
        if ((size_t)offset > BLOCK_SIZE) {
            *errnoptr = EFBIG;
            return -1;
        }

        //0 fill extra bytes
        memset((char*)data_ptr + file_node->size, 0, bytes_to_zero);

        //Update size
        file_node->size = offset;
        //Update time
        file_node->mod_time = file_node->chng_time = time(NULL);
        return 0;
    }
    return 0;
}

// Implements an emulation of the open system call on the filesystem 
//    of size fssize pointed to by fsptr, without actually performing the opening
//    of the file (no file descriptor is returned).

//    The call just checks if the file (or directory) indicated by path
//    can be accessed, i.e. if the path can be followed to an existing
//    object for which the access rights are granted.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The two only interesting error codes are 

//    * EFAULT: the filesystem is in a bad state, we can't do anything

//    * ENOENT: the file that we are supposed to open doesn't exist (or a
//              subpath).

//    It is possible to restrict ourselves to only these two error
//    conditions. It is also possible to implement more detailed error
//    condition answers.

//    The error codes are documented in man 2 open.

int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Check path input is valid 
    if (!path ||!strlen(path)) {
        *errnoptr = EINVAL;
        return -1;
    }

    //searching for node
    size_t node_offset;
    node *file_node = get_node(fsptr, fssize, path, &node_offset);
    if (!file_node) {
        *errnoptr = ENOENT;
        return -1;
    }

    //Check directory integrity is good
    if (file_node->mode & S_IFDIR) {
        directory_entry *entries = (directory_entry *)convert_off_to_ptr(fsptr, fssize, file_node->data_block);
        if (!entries) {
            *errnoptr = EIO; 
            return -1;
        }

        size_t num_entries = file_node->size / sizeof(directory_entry);
        if (num_entries < 2) {
            *errnoptr = EIO; 
            return -1;
        }
    }

    return 0;
}

// // Implements an emulation of the read system call on the filesystem 
//    of size fssize pointed to by fsptr.

//    The call copies up to size bytes from the file indicated by 
//    path into the buffer, starting to read at offset. See the man page
//    for read for the details when offset is beyond the end of the file etc.

//    On success, the appropriate number of bytes read into the buffer is
//    returned. The value zero is returned on an end-of-file condition.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 read.

int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, char *buf, size_t size, off_t offset) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT; 
        return -1;
    }

    //Check path and buffer
    if (!path || !buf) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Find node that belongs to the path
    size_t node_offset;
    node *file_node = get_node(fsptr, fssize, path, &node_offset);
    if (!file_node) {
        *errnoptr = ENOENT;
        return -1;
    }

    //Check node is a file
    if (!(file_node->mode & S_IFREG)) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Handle beyond EOF
    if (offset >= (off_t)file_node->size) {
        return 0;
    }

    //Check bytes to read
    size_t bytes_available = file_node->size - offset;
    size_t bytes_to_read = (size < bytes_available) ? size : bytes_available;

    //Check file has data block allocated
    if (!file_node->data_block) {
        *errnoptr = EIO;
        return -1;
    }

    //Check that offset is not bigger than block size
    if ((size_t)offset > BLOCK_SIZE) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Get ptr to data block at specified offset
    void *data_ptr = convert_off_to_ptr(fsptr, fssize, file_node->data_block + offset);
    if (!data_ptr) {
        *errnoptr = EIO;
        return -1;
    }

    //Copy data into buffer
    memcpy(buf, data_ptr, bytes_to_read);

    //Update nodes access time
    file_node->acc_time = time(NULL);

    //Return bytes read
    return (int)bytes_to_read;
}

// Implements an emulation of the write system call on the filesystem 
//    of size fssize pointed to by fsptr.

//    The call copies up to size bytes to the file indicated by 
//    path into the buffer, starting to write at offset. See the man page
//    for write for the details when offset is beyond the end of the file etc.

//    On success, the appropriate number of bytes written into the file is
//    returned. The value zero is returned on an end-of-file condition.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 write.

int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, const char *buf, size_t size, off_t offset) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT; 
        return -1;
    }

    //Find file node to write to
    size_t node_offset;
    node *file_node = get_node(fsptr, fssize, path, &node_offset);
    if (!file_node) {
        *errnoptr = ENOENT;
        return -1;
    }

    //Check if file is accessable
    if (!(file_node->mode & S_IFREG)) {
        *errnoptr = EINVAL; 
        return -1;
    }

    //Allocate data block memory
    if (file_node->data_block == 0) {
        size_t data_block_offset = find_free_data_block(fsptr, fssize);
        if (data_block_offset == (size_t)-1) {
                *errnoptr = ENOSPC;
                return -1;
        }
        file_node->data_block = data_block_offset;
    }

    //Get offset to block
    void *data_ptr = convert_off_to_ptr(fsptr, fssize, file_node->data_block + offset);
    if (!data_ptr) {
        *errnoptr = EIO;
        return -1;
    }

    //Check if write > block size
    if (offset + size > BLOCK_SIZE) {
        *errnoptr = EFBIG; // File too large
        return -1;
    }

    //Write to buffer
    memcpy(data_ptr, buf, size);

    //Update metadata
    if (offset + size > file_node->size) file_node->size = offset + size;
    file_node->mod_time = file_node->chng_time = time(NULL);

    //Return bytes written
    return size;
}


// Implements an emulation of the utimensat system call on the filesystem 
//    of size fssize pointed to by fsptr.

//    The call changes the access and modification times of the file
//    or directory indicated by path to the values in ts.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 utimensat.


int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr, const char *path, const struct timespec ts[2]) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT;
        return -1;
    }

    //Check path is valid 
    if (!path|| !strlen(path)) {
        *errnoptr = EINVAL;
        return -1;
    }

    //Find path
    size_t node_offset;
    node *file_node = get_node(fsptr, fssize, path, &node_offset);
    if (!file_node) {
        *errnoptr = ENOENT;
        return -1;
    }

    //update access and modify times
    time_t new_acc_time;
    time_t new_mod_time;

    // Null, assign curr time
    if (!ts) {
        new_acc_time = new_mod_time = time(NULL);
    } else {
        //Set times vased on array
        new_acc_time = ts[0].tv_sec;
        new_mod_time = ts[1].tv_sec;

        //Check times
        if ((ts[0].tv_nsec < 0 || ts[0].tv_nsec >= 1000000000) ||
            (ts[1].tv_nsec < 0 || ts[1].tv_nsec >= 1000000000)) {
            *errnoptr = EINVAL;
            return -1;
        }
    }

    //Update times
    file_node->acc_time = new_acc_time;
    file_node->mod_time = new_mod_time;
    file_node->chng_time = time(NULL); 
    
    return 0;
}

// Implements an emulation of the statfs system call on the filesystem 
//    of size fssize pointed to by fsptr.

//    The call gets information of the filesystem usage and puts in 
//    into stbuf.

//    On success, 0 is returned.

//    On failure, -1 is returned and *errnoptr is set appropriately.

//    The error codes are documented in man 2 statfs.

//    Essentially, only the following fields of struct statvfs need to be
//    supported:__myfs_utimens_implem
//    f_bsize   fill with what you call a block (typically 1024 bytes)
//    f_blocks  fill with the total number of blocks in the filesystem
//    f_bfree   fill with the free number of blocks in the filesystem
//    f_bavail  fill with same value as f_bfree
//    f_namemax fill with your maximum file/directory name, if your
//              filesystem has such a maximum


int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr, struct statvfs* stbuf) {
    //Check that filesystem was initialized sucessfully
    if (!initialize_filesystem(fsptr, fssize)) {
        *errnoptr = EFAULT; 
        return -1;
    }

    //Check stat buffer
    if (!stbuf) {
        *errnoptr = EFAULT;
        return -1;
    }

    //0 stbuf
    memset(stbuf, 0, sizeof(struct statvfs));

    //Populate fields 
    stbuf->f_bsize = BLOCK_SIZE;
    stbuf->f_frsize = BLOCK_SIZE;
    stbuf->f_blocks = fssize - BLOCK_SIZE;
    stbuf->f_bfree = calculate_free_blocks(fsptr, fssize);
    stbuf->f_bavail = stbuf->f_bfree; 
    stbuf->f_namemax = MAX_FILENAME;

    return 0;
}