#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

// opens regular UNIX File
int openDisk(char *filename, int nBytes) {
    FILE *file;

    /* Opens file + designates first nBytes as space for emulated disk */
    if (nBytes = 0) {    
        /* Opens existing file and contents may not be overwritten */
        file = fopen(filename, "r");     // should this be a+ or just r?

        // find the disk entry in the list and set it to open! 
    }
    else if(nBytes < BLOCKSIZE) {
        /* Issue with nBytes, Return error code */
        return ERR_NOFILE;
    } else {
        /* Already file given by filename, the file's content may be overwritten */
        if (access(filename, F_OK) == 0) {  /* file should exist */
            file = fopen(filename, "w+");
        } else {
            return ERR_NOFILE; 
        }

        amount = nBytes;
        if (nBytes % BLOCKSIZE != 0) {
            /* if nbytes not multiple of blocksize then set it to the closest multiple */
            amount = nBytes / BLOCKSIZE * BLOCKSIZE;
        } 

        // add file as disk into list now using amount as its size!! 
    }

    if !(file) {    /* confirming that the file actually was oepned */
        return ERR_NOFILE; 
    }


    // returns disk number on success + maybe if disk already exists return that disk # instead?
    return diskCount++;
}

int closeDisk(int disk) {
    File *file;
    // go into our linked list structure, find node where diskNumber matches disk
    // get that file name and set the file to closed 

    fclose(file);
    return 0;
}

int readBlock(int disk, int bNum, void *block) {
    FILE *file;
    // go into our linked list structure check that disk exists 
    // get the disk file name out 
    file = disk_file_name;

    /* Check that block is the size of a BLOCKSIZE  */ 
    if (block != NULL && sizeof(block) >= 256) || (block == NULL) {
        return ERR_RBLOCKISSUE;
    }

    startByte = bNum * BLOCKSIZE;
    // check that startByte is not greater than the size of the file 

    // go into our file and read the block 
    if (fseek(file, startByte, SEEK_SET) != 0) {
        return ERR_RSEEKISSUE;
    }

    size_t bytesRead = fread(block, 1, BLOCKSIZE, file);
    if (bytesRead != BLOCKSIZE) {
        return ERR_READISSUE;
    }

    // read block from list, indicate disk integer
    // and then say what block you want to read from 
    // void *block = write content in here 

     /* readBlock() reads an entire block of BLOCKSIZE bytes from the open
    disk (identified by ‘disk’) and copies the result into a local buffer
    (must be at least of BLOCKSIZE bytes). The bNum is a logical block
    number, which must be translated into a byte offset within the disk. The
    translation from logical to physical block is straightforward: bNum=0
    is the very first byte of the file. bNum=1 is BLOCKSIZE bytes into the
    disk, bNum=n is n*BLOCKSIZE bytes into the disk. On success, it returns
    0. -1 or smaller is returned if disk is not available (hasn’t been
    opened) or any other failures. You must define your own error code
    system. */
    return 0; 
}

int writeBlock(int disk, int bNum, void *block) {
    FILE *file;
    // go into our linked list structure check that disk exists 
    // get the disk file name out 
    file = disk_file_name;

    /* Check that block is the size of a BLOCKSIZE */ 
    if (block != NULL && sizeof(block) == 256) || (block == NULL) {
        return ERR_WBLOCKISSUE;
    }

    startByte = bNum * BLOCKSIZE;
    // check that startByte is not greater than the size of the file 

    resultByte = startByte + BLOCKSIZE 
    // check that the result will not be greater than the end of file 
    
    // writes to file 
    if (fseek(file, startByte, SEEK_SET) != 0) {    /* moves head of file to startByte */
        return ERR_WSEEKISSUE;
    }

    size_t bytesWritten = fwrite(block, 1, BUFFER_SIZE, file);
    if (bytesWritten != BLOCKSIZE) {
        return ERR_WRITEISSUE;
    }
   
    /* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’
and writes the content of the buffer ‘block’ to that location. ‘block’
must be integral with BLOCKSIZE. Just as in readBlock(), writeBlock()
must translate the logical block bNum to the correct byte position in
the file. On success, it returns 0. -1 or smaller is returned if disk
is not available (i.e. hasn’t been opened) or any other failures. You
must define your own error code system. */
    return 0;
}
