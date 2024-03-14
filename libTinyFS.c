#include <math.h>

#include "libTinyFS.h"
#include "tinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"



int curDisk = -1;
FileDetails *resourceTable[NUM_BLOCKS - 1] = {NULL};
int resourceTablePointer = 0;



void print_disk(int diskNum) {
    int i;
    int block[BLOCKSIZE];
    printf("\nDISK\n-------------------------\n");
    for (i = 0; i < NUM_BLOCKS; i++) {
        readBlock(diskNum, i, block);
        printf("type: %d, link: %d\n", block[0], block[2]);
    }
    printf("-------------------------\n\n");
}

void print_rt() {
    int i;
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        if (resourceTable[i] != NULL) {
            printf("file descriptor: %d, name: %s\n", resourceTable[i]->fd, resourceTable[i]->name);
        }
    }
}

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

// Given a num and buffer, add that num of free blocks to the buffer and remove them from the free block chain
// Return 0 on success or error code on failure
int fbc_get(int num, int* buffer) {
    // Init variables
    int status, i;
    int curBlock[BLOCKSIZE];

    // For every free block
    for (i = 0; i < num; i++) {
        // Read block
        if (i == 0) {
            status = readBlock(curDisk, 0, curBlock);
        } else {
            status = readBlock(curDisk, curBlock[2], curBlock);
        }
        if (status < 0) {
            return status;
        }

        // Check that there's free blocks available
        if (curBlock[2] == 0) {
            return ERR_FULLDISK;
        }

        // Add free block
        buffer[i] = curBlock[2];
    }

    // Update superblock to point to the next free block
    int nextFreeBlock = curBlock[2];
    // Read superblock
    status = readBlock(curDisk, 0, curBlock);
    if (status < 0) {
        return status;
    }
    // Update pointer
    curBlock[2] = nextFreeBlock;

    // Finished successfully
    return 0;
}

// Given a diskNum, num, and buffer, add that num of free blocks in the buffer to the free block chain
// Return 0 on success or error code on failure
int fbc_set(int diskNum, int num, int* buffer) {
    // Init variables
    int status, i, lastBlockNum = 0;
    int curBlock[BLOCKSIZE], freeBlock[BLOCKSIZE];

    // Read superblock
    status = readBlock(diskNum, 0, curBlock);
    if (status < 0) {
        return status;
    }

    // Get to the last free block
    while (curBlock[2] != 0) {
        lastBlockNum = curBlock[2];
        status = readBlock(diskNum, curBlock[2], curBlock);
        if (status < 0) {
            return status;
        }
    }

    // Update the block's link
    curBlock[2] = buffer[0];
    status = writeBlock(diskNum, lastBlockNum, curBlock);

    // For every free block
    for (i = 0; i < num; i++) {
        // Create free block
        if (i < num - 1) {
            create_block(freeBlock, FREEBLOCK, buffer[i + 1], NULL, 0);
        } else {
            create_block(freeBlock, FREEBLOCK, 0, NULL, 0);
        }
        // Write free block to disk
        status = writeBlock(diskNum, buffer[i], freeBlock);
        if (status < 0) {
            return status;
        }
    }

    // Finished successfully
    return 0;
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

// Given a file descriptor, get the index of that file in the resource table
// Return index on success or error code on failure
int get_file_idx(fileDescriptor fd) {
    // Init variables
    int i, idx = 0;

    // Set i to the index of the open file in our resource table
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        if (resourceTable[i] && resourceTable[i]->fd == fd) {
            idx = i;
            break;
        }
    }

    // Couldn't find file
    if (idx == 0) {
        return ERR_NOFILE;
    }

    // Found file, return the idx
    return idx;
}

// Given a filename and number of bytes, create a filesystem of size nBytes on the filename
// Return the disk number on success or error code on failure
int tfs_mkfs(char *filename, int nBytes) {
    // Init variables
    int i, status;
    int freeBlocks[NUM_BLOCKS - 1];
    for (i = 1; i < NUM_BLOCKS; i++) {
        freeBlocks[i - 1] = i;
    }

    // Make a disk on the file
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) {
        return ERR_NOFILE;
    }

    // Create the superblock
    int superblock[BLOCKSIZE];
    create_block(superblock, SUPERBLOCK, 0, NULL, 0);
    // Write superblock to the disk
    status = writeBlock(diskNum, 0, superblock);
    if (status < 0) {
        return ERR_WBLOCKISSUE;
    }
    
    // Write free blocks
    status = fbc_set(diskNum, NUM_BLOCKS - 1, freeBlocks);
    if (status < 0) {
        return status;
    }

    // PRINT TESTING
    printf("\nMade disk %d\n", diskNum);
    print_disk(diskNum);

    // Close the disk
    closeDisk(diskNum);

    // Return the disk number on success
    return diskNum;
}

// Given a disk name that contains a file system, mount it
// Return disk number on success or error code on failure
int tfs_mount(char *diskname) {
    // Init variables
    int i, status;
    
    // Ensure that there's not already a disk mounted
    if (curDisk != -1) {
        return ERR_MOUNTMULTIPLE;
    }

    // Open disk
    int diskNum = openDisk(diskname, 0);
    if (diskNum < 0) {
        return diskNum;
    }

    // PRINT TESTING
    printf("Opening disk %d to mount\n", diskNum);

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

    // PRINT TESTING
    printf("File is formatted correctly\n");
    print_disk(curDisk);
    printf("Mounted disk %d\n\n", curDisk);

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
/* Steps: 
1. Create the file's details and add it to the resource table
2. Update the resource table pointer
3. If the file doesn't exist, do:
    4. Create an inode block
    5. Update the free block list pointer in the superblock
    6. Write the inode block to the disk at the previous free block list pointer */
fileDescriptor tfs_openFile(char *name) {
    // Init variables
    int i, status, startBlock = 0, fileExists = 0;
    int buffer[1];

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
                startBlock = i;
                break;
            }
        }
    }

    // PRINT TESTING
    if (fileExists) {
        printf("\nOpening existing file %s\n\n", name);
    } else {
        printf("\nCreating nonexisting file %s\n", name);
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
    file->blockNum = startBlock;

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
        // Get next free block
        status = fbc_get(1, buffer);
        if (status < 0) {
            free(file);
            return status;
        }

        // Add inode block to disk
        int inodeBlock[BLOCKSIZE];
        // Write name and size to inode block
        int inodeData[DATASIZE];
        for (i = 0; name[i] != 0; i++) {
            inodeData[i] = name[i];
        }
        if (i > MAXNAMECHARS) {
            free(file);
            return ERR_FILENAMELIMIT;
        }
        inodeData[i++] = 0;
        inodeData[i] = 0;

        // Create the inode block and write it to the disk
        create_block(inodeBlock, INODE, 0, inodeData, ++i);
        status = writeBlock(curDisk, buffer[0], inodeBlock);
        if (status < 0) {
            // SOMETHING GOES WRONG HERE
            free(file);
            return (status);
        }

        // PRINT TESTING
        printf("Created file with inode: %d, name: %s, fd: %d, blockNum: %d\n\n",
        file->inode, file->name, file->fd, file->blockNum);
    }

    // PRINT TESTING
    print_disk(curDisk);
    pause();

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
    // Init variables
    int status, emptyBlocks = 0;

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) {
        return idx;
    }

    // Number of blocks needed
    int numBlocks = floor(size / DATASIZE);
    if (size % DATASIZE != 0) {
        numBlocks++;
    }

    // Number of empty blocks

    // Read the superblock
    int curBlock[BLOCKSIZE];
    status = readBlock(curDisk, 0, curBlock);
    if (status < 0) {
        return status;
    }
    // Read every empty block in the empty block chain
    while (curBlock[2]) {
        emptyBlocks++;
        status = readBlock(curDisk, curBlock[2], curBlock);
        if (status < 0) {
            return status;
        }
    }
    // Add the file's blocks that we're going to overwrite to the number of empty blocks
    // Read inode block
    status = readBlock(curDisk, resourceTable[idx]->blockNum, curBlock);
    if (status < 0) {
        return status;
    }
    // Read every file extent block
    while (curBlock[2]) {
        emptyBlocks++;
        status = readBlock(curDisk, curBlock[2], curBlock);
        if (status < 0) {
            return status;
        } else if (curBlock[0] != FILEEXTENT) {
            return ERR_BLOCKFORMAT;
        }
    }

    // PRINT TESTING
    printf("Trying to write %d blocks with %d empty blocks\n", numBlocks, emptyBlocks);

    // Ensure that there's enough empty blocks to write the file

    // Set all file's blocks to empty (not inode)
    // See how many blocks are needed
    // See how many empty blocks there are
    // If there are enough empty blocks, write the file
    // Set the file pointer to 0
    printf("test\n");
    return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
    // Set all file's blocks to empty (including inode)
    // Add empty blocks to the free block list
    return 0;
}

// Read a byte into the given buffer from a file at its pointer and increment the pointer
// Return 0 on success or error code on failure
int tfs_readByte(fileDescriptor FD, char *buffer) {
    // Init variables
    int i, status, size, blockNum, offset;

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) {
        return idx;
    }

    // Read inode block
    int block[BLOCKSIZE];
    status = readBlock(curDisk, resourceTable[idx]->blockNum, block);
    if (status < 0) {
        return status;
    }

    // Get the size of the data
    i = 4;
    while (block[i]) {
        i++;
    }
    size = block[++i];

    // Check that file pointer is within range
    if (resourceTable[idx]->filePointer >= size) {
        return ERR_RSEEKISSUE;
    }

    // Set the block number and offset
    blockNum = floor(resourceTable[idx]->filePointer / DATASIZE);
    offset = resourceTable[idx]->filePointer % DATASIZE;

    // Get to the right block
    for (i = 0; i < blockNum; i++) {
        status = readBlock(curDisk, block[2], block);
        if (status < 0) {
            return status;
        }
        if (block[0] != FILEEXTENT) {
            return ERR_BLOCKFORMAT;
        }
    }

    // Read byte based on offset
    *buffer = block[4 + offset];

    // PRINT TESTING
    printf("Read byte %c from file %s\n", *buffer, resourceTable[idx]->name);
    printf("File pointer moved from %d to %d\n\n", resourceTable[idx]->filePointer - 1, resourceTable[idx]->filePointer);
    pause();

    // Increment file pointer
    resourceTable[idx]->filePointer++;

    // Return 0 on success
    return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
    // Change the file pointer location
    if (offset > BLOCKSIZE - 1) {
        return ERR_WSEEKISSUE;
    }
    return 0;
}
