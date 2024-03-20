#include <math.h>

#include "libTinyFS.h"
#include "tinyFS.h"
#include "libDisk.h"
#include "TinyFS_errno.h"



int curDisk = -1;
FileDetails *resourceTable[NUM_BLOCKS - 1] = {NULL};
int resourceTablePointer = 0;
char *typeMap[4] = {"superblock", "inode", "file extent", "free block"};



void print_disk(int diskNum, int numBlocks, int dataSize) {
    int i, j, status;
    char block[BLOCKSIZE] = {0};
    printf("\nDISK %d\n-------------------------\n", diskNum);
    for (i = 0; i < numBlocks; i++) {
        status = readBlock(diskNum, i, block);
        if (status < 0) {
            perror("print_disk");
            exit(1);
        }
        printf("num: %2d   |   type: %11s   |   link: %2d\n", i, typeMap[(int) block[0] - 1], block[2]);
        if (dataSize) {
            for (j = 0; j < dataSize; j++) {
                printf("%d ", block[j]);
            }
            printf("\n\n");
        }
    }
    printf("-------------------------\n\n");
}

void print_rt() {
    int i;
    printf("\nRESOURCE TABLE\n-------------------------\n");
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        if (resourceTable[i] != NULL) {
            printf("file descriptor: %d, name: %s, block num: %d\n", resourceTable[i]->fd, resourceTable[i]->name, resourceTable[i]->inode);
        }
    }
    printf("-------------------------\n\n");
}

// Given a pointer to a block, a type, magic, link_addr, data, and data size, create the block
void create_block(char *block, int type, int link_addr, char *data, int data_size) {
    // Init type, magic, and link_addr
    int i;
    for (i = 0; i < BLOCKSIZE; i++) {
        block[i] = 0;
    }
    block[0] = type;
    block[1] = MAGIC;
    if (link_addr) {
        block[2] = link_addr;
    }
    // Write the data
    if (data && data_size > 0) {
        // Copy over data
        memcpy(block + 4, data, data_size);
    }
}

// Given a num and buffer, add that num of free blocks to the buffer and remove them from the free block chain
// Return 0 on success or error code on failure
int fbc_get(int num, int* buffer) {
    // Init variables
    int status, i;
    char curBlock[BLOCKSIZE];

    // For every free block
    for (i = 0; i < num; i++) {
        // Read block
        if (i == 0) {
            status = readBlock(curDisk, 0, curBlock);
        } else {
            status = readBlock(curDisk, curBlock[2], curBlock);
        }
        if (status < 0) return status;

        // Check that there's free blocks available
        if (curBlock[2] == 0) return ERR_FULLDISK;

        // Add free block
        buffer[i] = curBlock[2];
    }

    // Get the next next free block
    status = readBlock(curDisk, curBlock[2], curBlock);
    if (status < 0) return status;
    int nextFreeBlock = curBlock[2];

    // Read superblock
    status = readBlock(curDisk, 0, curBlock);
    if (status < 0) return status;
    // Update pointer
    curBlock[2] = nextFreeBlock;

    // Write superblock
    status = writeBlock(curDisk, 0, curBlock);
    if (status < 0) return status;

    // Finished successfully
    return 0;
}

// Given a diskNum, num, and buffer, add that num of free blocks in the buffer to the free block chain
// Return 0 on success or error code on failure
int fbc_set(int diskNum, int num, int* buffer) {
    // Init variables
    int i, lastBlockNum = 0;
    char curBlock[BLOCKSIZE], freeBlock[BLOCKSIZE];

    // No free blocks to add
    if (!num) return 0;

    // Read superblock
    int status = readBlock(diskNum, 0, curBlock);
    if (status < 0) return status;

    // Get to the last free block
    while (curBlock[2]) {
        lastBlockNum = curBlock[2];
        status = readBlock(diskNum, curBlock[2], curBlock);
        if (status < 0) return status;
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
        if (status < 0) return status;
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
    int i;

    // Set i to the index of the open file in our resource table
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        if (resourceTable[i]) {
            if (resourceTable[i]->fd == fd) return i;
        }
    }

    // Couldn't find the file
    return ERR_NOFILE;
}

// Given a resource table index, get the size of the file
// Return the size on success or error code on failure
int get_fileSize(int idx) {
    // Read inode block
    char block[BLOCKSIZE];
    int status = readBlock(curDisk, resourceTable[idx]->inode, block);
    if (status < 0) return status;

    // Get the size of the data
    char charSize[SIZELENGTH] = {0};
    memcpy(charSize, block + 13, SIZELENGTH);

    // Return the size on success
    return atol(charSize);
}

// Given a diskNum, init the disk with a superblock and free blocks
// Return 0 on success or error code on failure
int initDisk(int diskNum) {
    // Init variables
    int i;
    int freeBlocks[NUM_BLOCKS - 1];
    for (i = 1; i < NUM_BLOCKS; i++) {
        freeBlocks[i - 1] = i;
    }

    // Create the superblock
    char superblock[BLOCKSIZE];
    create_block(superblock, SUPERBLOCK, 0, NULL, 0);
    // Write superblock to the disk
    int status = writeBlock(diskNum, 0, superblock);
    if (status < 0) return status;
    
    // Write free blocks
    status = fbc_set(diskNum, NUM_BLOCKS - 1, freeBlocks);
    if (status < 0) return status;

    // Finished successfully
    return 0;
}

// Given a block, time type, and time, set that time in the block
// Return 0 on success or error code on failure
int setTime(char *block, char *type, time_t curTime) {
    // Init variables
    char timeStr[TIMELENGTH] = {0};
    sprintf(timeStr, "%ld", curTime);

    // Update the time depending on the type
    if (!strcmp(type, "creation")) {
        memcpy(block + 19, timeStr, TIMELENGTH);
    } else if (!strcmp(type, "modification")) {
        memcpy(block + 30, timeStr, TIMELENGTH);
    } else if (!strcmp(type, "access")) {
        memcpy(block + 41, timeStr, TIMELENGTH);
    } else {
        return ERR_TIMING;
    }

    // Finished successfully
    return 0;
}

// Given a block, time type, and time, get that time in the block
// Return the time on success or error code on failure
int getTime(char *block, char *type) {
    // Init variables
    char timeStr[TIMELENGTH] = {0};

    // Update the time depending on the type
    if (!strcmp(type, "creation")) {
        memcpy(timeStr, block + 19, TIMELENGTH);
    } else if (!strcmp(type, "modification")) {
        memcpy(timeStr, block + 30, TIMELENGTH);
    } else if (!strcmp(type, "access")) {
        memcpy(timeStr, block + 41, TIMELENGTH);
    } else {
        return ERR_TIMING;
    }

    // Finished successfully
    return atol(timeStr);
}

// Given a filename and number of bytes, create a filesystem of size nBytes on the filename
// Return the disk number on success or error code on failure
int tfs_mkfs(char *filename, int nBytes) {
    // Init variables
    int i;

    // PRINT TESTING
    // printf("tfs_mkfs\n");

    // Make a disk on the file
    int diskNum = openDisk(filename, nBytes);
    if (diskNum < 0) return diskNum;

    // Initialize the disk to all zeros
    char emptyBlock[BLOCKSIZE] = {0};
    for (i = 0; i < nBytes / BLOCKSIZE; i++) {
        writeBlock(diskNum, i, emptyBlock);
    }

    // Init the disk blocks
    int status = initDisk(diskNum);
    if (status < 0) return status;

    // Set to the current disk
    curDisk = diskNum;

    // Close the disk
    status = closeDisk(diskNum);
    if (status < 0) return status;

    // Return the disk number on success
    return diskNum;
}

// Given a disk name that contains a file system, mount it
// Return disk number on success or error code on failure
int tfs_mount(char *diskname) {
    // Init variables
    int i, status;

    // PRINT TESTING
    // printf("tfs_mount\n");

    // Open disk
    int diskNum = openDisk(diskname, DEFAULT_DISK_SIZE);
    if (diskNum < 0) return diskNum;

    // Ensure that file system is formatted correctly
    char block[BLOCKSIZE];
    // Iterate through each block
    for (i = 0; i < NUM_BLOCKS; i++) {
        // Read block
        status = readBlock(diskNum, i, block);
        if (status < 0) {
            closeDisk(diskNum);
            return status;
        }
        // Check the superblock
        if (i == 0) {
            if (block[0] != SUPERBLOCK || block[2] > NUM_BLOCKS - 1 || block[2] < 0 || block[3] != 0x00) {
                closeDisk(diskNum);
                return ERR_BLOCKFORMAT;
            }
        }
        // Check the magic number
        if (block[1] != MAGIC) {
            closeDisk(diskNum);
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
    // PRINT TESTING
    // printf("tfs_unmount\n");

    // Close disk
    int status = closeDisk(curDisk);
    if (status < 0) return status;

    // Unmount disk
    curDisk = -1;
    return 0;
}

// Create or open a file for "rw"
// Return a file descriptor on success or error code on failure
fileDescriptor tfs_openFile(char *name) {
    // Init variables
    int i, status, startBlock = 0, fileExists = 0;
    int buffer[1];

    // PRINT TESTING
    // printf("tfs_openFile\n");

    // Check that the length of the filename is within bounds
    if (strlen(name) > 8) return ERR_FILENAMELIMIT;

    // Check if file already exists
    char curBlock[BLOCKSIZE];
    for (i = 1; i < NUM_BLOCKS - 1; i++) {
        // Read block
        status = readBlock(curDisk, i, curBlock);
        if (status < 0) return status;
        // Check if block is an inode block and if it's the file we're looking for
        if (curBlock[0] == INODE) {
            // Copy file name
            char fname[NAMELENGTH];
            memcpy(fname, curBlock + 4, NAMELENGTH);

            // If file exists
            if (strcmp(fname, name) == 0) {
                fileExists = 1;
                startBlock = i;
                break;
            }
        }
    }

    // Check that there are inodes available
    if (resourceTablePointer >= NUM_BLOCKS) return ERR_FULLDISK;

    // Create resource table entry
    FileDetails *file = malloc(sizeof(FileDetails));
    file->inode = startBlock;
    file->name = name;
    file->fd = resourceTablePointer;
    file->filePointer = 0;
    file->rw = 1;

    // If the file exists, update the access time
    if (fileExists) {
        // Update the access time
        time_t curTime;
        if (time(&curTime) == -1) {
            free(file);
            return ERR_TIMING;
        }
        status = setTime(curBlock, "access", curTime);
        if (status < 0) {
            free(file);
            return status;
        }

        // Write the updated inode block
        status = writeBlock(curDisk, startBlock, curBlock);
        if (status < 0) {
            free(file);
            return status;
        }
    }

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
        // Update the file's time info
        time_t curTime;
        if (time(&curTime) == -1) {
            free(file);
            return ERR_TIMING;
        }

        // Get next free block
        status = fbc_get(1, buffer);
        if (status < 0) {
            free(file);
            return status;
        }

        // Add inode block to disk
        char inodeBlock[BLOCKSIZE];
        // Write name to inode block
        char inodeData[DATASIZE] = {0};
        memcpy(inodeData, name, strlen(name));

        // Create the inode block and write it to the disk
        create_block(inodeBlock, INODE, 0, inodeData, ++i);
        // Write the times
        status = setTime(inodeBlock, "creation", curTime);
        if (status < 0) return status;
        status = setTime(inodeBlock, "modification", curTime);
        if (status < 0) return status;
        status = setTime(inodeBlock, "access", curTime);
        if (status < 0) return status;

        // Write the inode block to the disk
        status = writeBlock(curDisk, buffer[0], inodeBlock);
        if (status < 0) {
            free(file);
            return (status);
        }

        // Set the file's block number in the resource table
        file->inode = buffer[0];
    }

    // Return file descriptor
    return file->fd;
}

// Close a file and remove it from the resource table
// Return 0 on success or error code on failure
int tfs_closeFile(fileDescriptor FD) {
    // Init variables
    int i, status;

    // PRINT TESTING
    // printf("tfs_closeFile\n");

    // Find the file
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        // File found
        if (resourceTable[i] && resourceTable[i]->fd == FD) {
            // Free the resource table entry and set it to null
            free(resourceTable[i]);
            resourceTable[i] = NULL;
            // Update the resource table pointer
            status = update_rt_pointer();
            if (status < 0) return status;

            // Successfully closed file
            return 0;
        }
    }

    // Couldn't find the file
    return ERR_NOFILE;
}

// Write data of size from the buffer into a file, overwriting previous data and update the inode block
// Returns 0 on success and error code on failure
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    // Init variables
    int curSize, i = 0;
    char curBlock[BLOCKSIZE] = {0}, superblock[BLOCKSIZE] = {0}, inodeBlock[BLOCKSIZE] = {0};
    int curDataBlocks[NUM_BLOCKS - 2];

    // PRINT TESTING
    // printf("tfs_writeFile\n");

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) return idx;

    // Check that we have write permissions
    if (!resourceTable[idx]->rw) return ERR_READONLY;

    // Set the file pointer to 0
    resourceTable[idx]->filePointer = 0;

    // Get the file's inode block and initialize the current block
    int status = readBlock(curDisk, resourceTable[idx]->inode, inodeBlock);
    if (status < 0) return status;
    memcpy(curBlock, inodeBlock, BLOCKSIZE);

    // Get the superblock
    status = readBlock(curDisk, 0, superblock);
    if (status < 0) return status;

    // Update inode block's link
    inodeBlock[2] = superblock[2];
    // Update the inode block's data size
    char charSize[SIZELENGTH] = {0};
    sprintf(charSize, "%d", size);
    memcpy(inodeBlock + 13, charSize, SIZELENGTH);
    // Update the inode block's modification and access time
    time_t curTime;
    if (time(&curTime) == -1) return ERR_TIMING;
    status = setTime(inodeBlock, "modification", curTime);
    if (status < 0) return status;
    status = setTime(inodeBlock, "access", curTime);
    if (status < 0) return status;

    // Iterate through each data block to get their block numbers and total number of blocks
    while (curBlock[2]) {
        curDataBlocks[i++] = curBlock[2];
        status = readBlock(curDisk, curBlock[2], curBlock);
        if (status < 0) return status;
    }

    // Free the file's current data blocks
    fbc_set(curDisk, i, curDataBlocks);

    // Write the new inode block
    status = writeBlock(curDisk, resourceTable[idx]->inode, inodeBlock);
    if (status < 0) return status;

    // Number of blocks needed
    int numBlocks = ceil((float) size / (float) DATASIZE);
    int freeBlocks[numBlocks];

    // Get next free blocks
    status = fbc_get(numBlocks, freeBlocks);
    if (status < 0) return status;

    // Create and write all file extent blocks
    for (i = 0; i < numBlocks; i++) {
        // Reset the cur block
        memset(curBlock, 0, BLOCKSIZE);

        // Get the size of the data to write to the block
        if (i != numBlocks - 1) {
            curSize = DATASIZE;
        } else {
            curSize = size % DATASIZE;
        }

        // Copy data of the current size from the buffer to write to the block
        char dataBuffer[curSize];
        memcpy(dataBuffer, buffer + i * DATASIZE, curSize);

        // Create the file extent block
        if (i != numBlocks - 1) {
            create_block(curBlock, FILEEXTENT, freeBlocks[i + 1], dataBuffer, curSize);
        } else {
            create_block(curBlock, FILEEXTENT, 0, dataBuffer, curSize);
        }

        // Write the file extent block
        writeBlock(curDisk, freeBlocks[i], curBlock);
    }

    // Finished successfully
    return 0;
}

// Set a file's blocks in the disk to free
// Return 0 on success or error code on failure
int tfs_deleteFile(fileDescriptor FD) {
    // Init variables
    int i = 0;
    int fileBlocks[NUM_BLOCKS - 1];
    char curBlock[BLOCKSIZE];

    // PRINT TESTING
    // printf("tfs_deleteFile\n");
    
    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) return idx;

    // Check that we have write permissions
    if (!resourceTable[idx]->rw) return ERR_READONLY;

    // Add the inode block to the file blocks list
    fileBlocks[i++] = resourceTable[idx]->inode;

    // Get the inode block
    int status = readBlock(curDisk, resourceTable[idx]->inode, curBlock);
    if (status < 0) return status;

    // Get every data block
    while (curBlock[2]) {
        fileBlocks[i++] = curBlock[2];
        status = readBlock(curDisk, curBlock[2], curBlock);
    }

    // Free file's blocks
    fbc_set(curDisk, i, fileBlocks);

    // Finished successfully
    return 0;
}

// Read a byte into the given buffer from a file at its pointer and increment the pointer
// Return 0 on success or error code on failure
int tfs_readByte(fileDescriptor FD, char *buffer) {
    // Init variables
    int i;

    // PRINT TESTING
    // printf("tfs_readByte\n");

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) return idx;

    // Read inode block
    char block[BLOCKSIZE];
    int status = readBlock(curDisk, resourceTable[idx]->inode, block);
    if (status < 0) return status;

    // Update the inode block's access time
    time_t curTime;
    if (time(&curTime) == -1) return ERR_TIMING;
    status = setTime(block, "access", curTime);
    if (status < 0) return status;

    // Write inode block
    status = writeBlock(curDisk, resourceTable[idx]->inode, block);
    if (status < 0) return status;

    // Get the size of the data
    int size = get_fileSize(idx);
    if (size < 0) return size;

    // Check that file pointer is within range
    if (resourceTable[idx]->filePointer > size || resourceTable[idx]->filePointer < 0) return ERR_RSEEKISSUE;

    // Set the block number and offset
    int blockNum = floor(resourceTable[idx]->filePointer / DATASIZE);
    int offset = resourceTable[idx]->filePointer % DATASIZE;

    // Get to the right block
    for (i = 0; i < blockNum + 1; i++) {
        status = readBlock(curDisk, block[2], block);
        if (status < 0) return status;
        if (block[0] != FILEEXTENT) return ERR_BLOCKFORMAT;
    }

    // Read byte based on offset
    *buffer = block[4 + offset];

    // Increment file pointer
    resourceTable[idx]->filePointer++;

    // Finished successfully
    return 0;
}

// Write a byte into a file at its pointer and increment the pointer
// Return 0 on success or error code on failure
int tfs_writeByte(fileDescriptor FD, unsigned int data) {
    // Init variables
    int i;

    // PRINT TESTING
    // printf("tfs_readByte\n");

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) return idx;

    // Check that we have write permissions
    if (!resourceTable[idx]->rw) return ERR_READONLY;

    // Read inode block
    char block[BLOCKSIZE];
    int status = readBlock(curDisk, resourceTable[idx]->inode, block);
    if (status < 0) return status;

    // Update the inode block's modification and access time
    time_t curTime;
    if (time(&curTime) == -1) return ERR_TIMING;
    status = setTime(block, "access", curTime);
    if (status < 0) return status;
    status = setTime(block, "access", curTime);
    if (status < 0) return status;

    // Write inode block
    status = writeBlock(curDisk, resourceTable[idx]->inode, block);
    if (status < 0) return status;

    // Get the size of the data
    int size = get_fileSize(idx);
    if (size < 0) return size;

    // Check that file pointer is within range
    if (resourceTable[idx]->filePointer > size || resourceTable[idx]->filePointer < 0) return ERR_RSEEKISSUE;

    // Set the block number and offset
    int diskBlock = 0;
    int blockNum = floor(resourceTable[idx]->filePointer / DATASIZE);
    int offset = resourceTable[idx]->filePointer % DATASIZE;

    // Get to the right block
    for (i = 0; i < blockNum + 1; i++) {
        diskBlock = block[2];
        status = readBlock(curDisk, block[2], block);
        if (status < 0) return status;
        if (block[0] != FILEEXTENT) return ERR_BLOCKFORMAT;
    }

    // Write byte based on offset
    block[4 + offset] = data;
    // Write block back to the disk
    status = writeBlock(curDisk, diskBlock, block);
    if (status < 0) return status;

    // Increment file pointer
    resourceTable[idx]->filePointer++;

    // Finished successfully
    return 0;
}

// Change the file pointer location to the offset (absolute)
// Return 0 on success or error code on failure
int tfs_seek(fileDescriptor FD, int offset) {
    // PRINT TESTING
    // printf("tfs_seek\n");

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) return idx;

    // Get the size of the data
    int size = get_fileSize(idx);
    if (size < 0) return size;

    // Check that the offset is within bounds
    if (offset < 0 || offset >= size) return ERR_OUTOFBOUNDS;

    // Change the file pointer location
    resourceTable[idx]->filePointer = offset;

    // Finished successfully
    return 0;
}

// Display a map of the free and occupied blocks in the disk
void tfs_displayFragments() {
    print_disk(curDisk, NUM_BLOCKS, 0);
}

// Move all the blocks so that the free blocks are continuous at the end of the disk
// Return 0 on success or error code on failure
int tfs_defrag() {
    // Init variables
    int i, status, pointer = 0;
    char blockList[NUM_BLOCKS][256];
    char block[BLOCKSIZE];

    // Iterate through every block and save the inode/file extent blocks
    for (i = 0; i < NUM_BLOCKS; i++) {
        // Read the block
        status = readBlock(curDisk, i, block);
        if (status < 0) return status;

        // Check if an inode block
        if (block[0] == INODE) {
            // Add it to the block list
            memcpy(blockList[pointer++], block, BLOCKSIZE);

            // Iterate through file extent blocks
            while (block[2]) {
                // Read file extent block
                status = readBlock(curDisk, block[2], block);
                if (status < 0) return status;

                // Add it to the block list
                memcpy(blockList[pointer++], block, BLOCKSIZE);
            }
        }
    }

    // Reinit the disk
    status = initDisk(curDisk);
    if (status < 0) return status;

    // Get the number of free blocks we need to set
    int buffer[pointer];
    status = fbc_get(pointer, buffer);
    if (status < 0) return status;

    // Create and write every inode/file extent block to the disk
    for (i = 0; i < pointer; i++) {
        // Next block is an inode block or it's the last file extent block, set the link to 0
        if (blockList[i + 1][0] == INODE || i == pointer - 1) {
            blockList[i][2] = 0;
        } else {
            blockList[i][2] = i + 2;
        }
        // Write the block to the disk
        status = writeBlock(curDisk, i + 1, blockList[i]);
        if (status < 0) return status;
    }

    // Finished successfully
    return 0;
}

// Given a filename, make it readonly
// Return 0 on success or error code on failure
int tfs_makeRO(char *name) {
    // Init variables
    int i;
    
    // Find the file
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        // Entry exists
        if (resourceTable[i]) {
            // Correct file
            if (!strcmp(resourceTable[i]->name, name)) {
                // Set the bit to readonly
                resourceTable[i]->rw = 0;
                return 0;
            }
        }
    }

    // Couldn't find file
    return ERR_NOFILE;
}

// Given a filename, make it readwrite
// Return 0 on success or error code on failure
int tfs_makeRW(char *name) {
    // Init variables
    int i;
    
    // Find the file
    for (i = 0; i < NUM_BLOCKS - 1; i++) {
        // Entry exists
        if (resourceTable[i]) {
            // Correct file
            if (!strcmp(resourceTable[i]->name, name)) {
                // Set the bit to readwrite
                resourceTable[i]->rw = 1;
                return 0;
            }
        }
    }

    // Couldn't find file
    return ERR_NOFILE;
}

// Given a file descriptor, print out the data of that file
// Return 0 on success or error code on failure
int tfs_readFileInfo(fileDescriptor FD) {
    // Init variables
    char timeStr[MAXTIMESTRING] = {0};
    time_t t;

    // Get the resource table index of the open file
    int idx = get_file_idx(FD);
    if (idx < 0) return idx;

    // Get the inode block of the file
    char block[BLOCKSIZE] = {0};
    int status = readBlock(curDisk, resourceTable[idx]->inode, block);
    if (status < 0) return status;

    // Print out the data of the file
    printf("\nFILE INFORMATION\n");
    printf("-------------------------\n");
    printf("Link: %d\n", block[2]);

    printf("Name: %s\n", block + 4);
    
    printf("Size: ");
    if (!block[13]) {
        printf("0\n");
    } else {
        printf("%s\n", block + 13);
    }

    t = getTime(block, "creation");
    if (t < 0) return t;
    strftime(timeStr, MAXTIMESTRING, "%c", localtime(&t));
    printf("Creation Time:     %s\n", timeStr);

    t = getTime(block, "modification");
    if (t < 0) return t;
    strftime(timeStr, MAXTIMESTRING, "%c", localtime(&t));
    printf("Modification Time: %s\n", timeStr);

    t = getTime(block, "access");
    if (t < 0) return t;
    strftime(timeStr, MAXTIMESTRING, "%c", localtime(&t));
    printf("Access Time:       %s\n", timeStr);
    printf("-------------------------\n\n");

    // Finished successfully
    return 0;
}
