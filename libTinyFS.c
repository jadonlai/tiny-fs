#include "libTinyFS.h"
#include "tinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"



int* create_block(int size, int type, int magic, int* link_addr) {
    // Malloc memory for the block
    int* block = (int) malloc(sizeof(int) * size);
    if (!block) {
        return ERR_WBLOCKISSUE;
    }
    // Init all values to 0
    memset(block, 0x00, size);
    // Set block type
    block[0] = type;
    // Set magic number
    block[1] = magic;
    // Set link
    if (link_addr) {
        block[2] = *link_addr;
    }

    // NEED TO SET DATA SOMEHOW

    // Return
    return block;
}

// Given a filename and number of bytes, create a filesystem of size nBytes on the filename
int tfs_mkfs(char *filename, int nBytes) {
    // Init variables
    int i;

    // Open file
    FILE *file = fopen(filename, 'wb+');
    if (!file) {
        return ERR_NOFILE;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    int size = ftell(file);

    // Fill the file with 0x00
    fseek(file, 0, SEEK_SET);
    for (i = 0; i < size; i++) {
        fputc(0x00, file);
    }

    // NOT SURE WHAT TO MAKE THESE
    int rootInode = 0;
    int* pointerToListOfFreeBlocks = 0;
    // Create the superblock
    int* superblock = create_block(BLOCKSIZE, SUPERBLOCK, 0x44, rootInode);
    superblock[4] = pointerToListOfFreeBlocks;
    // Create the inodes
    // Create the file extent
    // Create the free block

    // Upon success, format the file to a mountable disk
    // Set the magic numbers
    // Initialize and write the superblock, inodes, file extent, and free block
    // Return a specified success/error code
    return 0;
}

int tfs_mount(char *diskname) {
    return 0;
}

int tfs_unmount(void) {
    return 0;
}

fileDescriptor tfs_openFile(char *name) {
    return 0;
}

int tfs_closeFile(fileDescriptor FD) {
    return 0;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
    return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
    return 0;
}
