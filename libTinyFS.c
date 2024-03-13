#include "libTinyFS.h"
#include "tinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"



int curDisk = -1;
FileDetails *resourceTable[NUM_BLOCKS - 1] = {NULL};
int resourceTablePointer = 0;



// Given a pointer to a block, a type, magic, link_addr, data, and data size, create the block
void create_block(int *block, int type, int link_addr, int *data, int data_size) {
    // Init type, magic, and link_addr
    int i;
    for (i = 0; i < BLOCKSIZE; i++) {
        block[i] = 0x00;
    }
    block[0] = type;
    block[1] = MAGIC;
    if (link_addr) {
        block[2] = link_addr;
    }
    // Write the data
    if (data && data_size > 0) {
        // Copy over data
        for (i = 0; i < data_size; i++) {
            block[i + 4] = data[i];
        }
    }
}

// Return the block number of the next free block or error code on failure
int get_free_block() {
    // Init variables
    int status;

    // Read superblock
    int superblock[BLOCKSIZE];
    status = readBlock(curDisk, 0, superblock);
    if (status < 0) {
        return status;
    }

    // Check that there's a free block available
    if (superblock[2] == 0) {
        return ERR_FULLDISK;
    }

    // Return next free block
    return superblock[2];
}

// Update the next free block to the next next free block
// Return the next next free block on success or error code on failure
int update_free_block() {
    // Init variables
    int i, status;

    // Read superblock
    int superblock[BLOCKSIZE];
    status = readBlock(curDisk, 0, superblock);
    if (status < 0) {
        return status;
    }

    int curBlock[BLOCKSIZE];
    // Iterate through every block
    for (i = 1; i < NUM_BLOCKS; i++) {
        // Read block
        status = readBlock(curDisk, i, curBlock);
        if (status < 0) {
            return status;
        }

        // Found free block, update the superblock and break
        if (curBlock[0] == FREEBLOCK) {
            // Update superblock
            superblock[2] = i;
            break;
        }
    }

    // If there's no free block, set the superblock's next free block to 0
    if (i == NUM_BLOCKS) {
        superblock[2] = 0;
    }
    writeBlock(curDisk, 0, superblock);

    // Return the next next free block or 0 if the disk is full
    return superblock[2];
}

// Update the resource table pointer to the next empty spot
// Return 0 on success or error code on failure
int update_rt_pointer() {
    // Init variables
    int i;

    // Find the next empty resource table spot
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        // Found an empty spot
        if (!resourceTable[i]) {
            resourceTablePointer = i;
            return 0;
        }
    }

    // Couldn't find empty spot
    return ERR_FULLDISK;
}

// Given a filename and number of bytes, create a filesystem of size nBytes on the filename
// Return the disk number on success or error code on failure
int tfs_mkfs(char *filename, int nBytes) {
    // Init variables
    int i;

    // Make a disk on the file
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) {
        return ERR_NOFILE;
    }

    // Create the superblock
    int block[BLOCKSIZE];
    create_block(block, SUPERBLOCK, 1, NULL, 0);
    // Write the superblock to the disk
    if (writeBlock(diskNum, 0, block) < 0) {
        return ERR_WBLOCKISSUE;
    }
    
    // Write free blocks
    for (i = 1; i < NUM_BLOCKS; i++) {
        create_block(block, FREEBLOCK, i, NULL, 0);
        if (writeBlock(diskNum, 0, block) < 0) {
            return ERR_WBLOCKISSUE;
        }
    }

    // Return the disk number on success
    return diskNum;
}

// Given a disk name that contains a file system, mount it
// Return disk number on success or error code on failure
int tfs_mount(char *diskname) {
    // Init variables
    int i, status;
    FILE *file = fopen(diskname, "r");
    if (!file) {
        return ERR_FILEISSUE;
    }

    // Ensure that there's not already a disk mounted
    if (curDisk == -1) {
        return ERR_MOUNTMULTIPLE;
    }

    // Open disk
    int diskNum = openDisk(diskname, 0);
    if (diskNum < 0) {
        return diskNum;
    }

    // Ensure that file system is formatted correctly
    int block[BLOCKSIZE];
    // Iterate through each block
    for (i = 0; i < NUM_BLOCKS; i++) {
        // Read block
        status = readBlock(diskNum, i, block);
        if (status < 0) {
            return status;
        }
        // Check the superblock
        if (i == 0) {
            if (block[0] != SUPERBLOCK || block[2] > NUM_BLOCKS - 1 || block[2] < 0 || block[3] != 0x00) {
                return ERR_BLOCKFORMAT;
            }
        }
        // Check the magic number
        if (block[1] != MAGIC) {
            return ERR_BLOCKFORMAT;
        }
    }

    // Mount disk
    curDisk = diskNum;
    return curDisk;
}

// Unmount the current disk
// Return 0 on success or error code on failure
int tfs_unmount(void) {
    // Close disk
    int status = closeDisk(curDisk);
    if (status < 0) {
        return status;
    }
    // Unmount disk
    curDisk = -1;
    return 0;
}

// Create or open a file for "rw"
// Return a file descriptor on success or error code on failure
fileDescriptor tfs_openFile(char *name) {
    // Init variables
    int i, status;
    int fileExists = 0;

    // Check if file already exists
    int curBlock[BLOCKSIZE];
    for (i = 1; i < NUM_BLOCKS - 1; i++) {
        // Read block
        status = readBlock(curDisk, i, curBlock);
        if (status < 0) {
            return status;
        }
        // Check if block is an inode block and if it's the file we're looking for
        if (curBlock[0] == INODE) {
            // If file exists
            if (strcmp((char *) curBlock + 4, name) == 0) {
                fileExists = 1;
            }
        }
    }

    // Check that there are inodes available
    if (resourceTablePointer >= NUM_BLOCKS) {
        return ERR_FULLDISK;
    }

    // Create resource table entry
    FileDetails *file = malloc(sizeof(FileDetails));
    file->inode = resourceTablePointer;
    file->name = name;
    file->fd = file->inode;
    file->filePointer = 0;

    // Add file to resource table
    resourceTable[resourceTablePointer] = file;

    // Update resource table pointer
    status = update_rt_pointer();
    if (status < 0) {
        free(file);
        return status;
    }

    // If the file doesn't exist, we need to create an inode block for it in the disk
    if (!fileExists) {
        // Add inode block to disk
        int inodeBlock[BLOCKSIZE];
        // Write name and size to inode block
        int inodeData[DATASIZE];
        strcpy((char *) inodeData, name);
        i = 0;
        while (inodeData[i]) {
            i++;
        }
        inodeData[++i] = 0;

        // Get the next free block
        int freeBlock = get_free_block();
        // Failure
        if (freeBlock < 0) {
            return freeBlock;
        // Memory is full, try to update the free block
        } else if (freeBlock == 0) {
            // If there's still no free block, return error
            if (update_free_block() == 0) {
                free(file);
                return ERR_FULLDISK;
            }
            // If there is a free block, get it
            freeBlock = get_free_block();
        }
        // Free block available

        // Update next free block
        status = update_free_block();
        if (status <= 0) {
            free(file);
            return ERR_FULLDISK;
        }

        // Create the inode block and write it to the disk
        create_block(inodeBlock, INODE, 0, inodeData, ++i);
        writeBlock(curDisk, freeBlock, inodeBlock);
    }

    // Return file descriptor
    return file->fd;
}

// Close a file
// Return 0 on success or error code on failure
int tfs_closeFile(fileDescriptor FD) {
    // Init variables
    int i, status;

    // Find the file
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        // File found
        if (resourceTable[i] && resourceTable[i]->fd == FD) {
            // Free the resource table entry and set it to null
            free(resourceTable[i]);
            resourceTable[i] = NULL;
            // Update the resource table pointer
            status = update_rt_pointer();
            if (status < 0) {
                return status;
            }
            // Successfully closed file
            return 0;
        }
    }

    // Couldn't find the file
    return ERR_NOFILE;
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
    if (offset > BLOCKSIZE - 1) {
        return ERR_WSEEKISSUE;
    }
    return 0;
}
