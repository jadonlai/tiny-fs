#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

Disk *head = NULL;

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

        int amount = nBytes;
        if (nBytes % BLOCKSIZE != 0) {
            /* if nbytes not multiple of blocksize then set it to the closest multiple */
            amount = nBytes / BLOCKSIZE * BLOCKSIZE;
        } 

        addDiskNode(diskCount, amount, filename); // will this return neg value if issue in addDiskNode
    }

    if !(file) {    /* confirming that the file actually was oepned */
        return ERR_NOFILE; 
    }


    // returns disk number on success + maybe if disk already exists return that disk # instead?
    return diskCount++;
}


void addDiskNode(int diskNumber, int diskSize, char *filename) {
    // loop through list until we get to end and make new Node (malloc) for disk 
    // here set the open-close status to open
}

Disk findDiskNode(int diskNumber) {
    // find Disk and send back DiskNode?? 
}

void changeDiskStatus(int diskNumber, int status) {
    // if disk is closed or open, status should be that value (0 = false, 1 = true)
    // find disk with diskNumber and change disk's status in node to int status 
}

int closeDisk(int disk) {
    Disk wanted_disk = findDiskNode(disk); /* Go into our linked list structure, find diskNode matching diskNum*/

    FILE *file = wanted_disk->file;
    fclose(file);

    changeDiskStatus(disk, 0); /* close disk in linkedlist */

    return 0;
}

int readBlock(int disk, int bNum, void *block) {
    Disk wanted_disk = findDiskNode(disk); /* Go into our linked list structure, find diskNode matching diskNum*/

    FILE *file = wanted_disk->file;

    /* Check that block is the size of a BLOCKSIZE  */ 
    if (block != NULL && sizeof(block) >= 256) || (block == NULL) {
        return ERR_RBLOCKISSUE;
    }

    startByte = bNum * BLOCKSIZE;
    /*  Check that startByte + BLOCKSIZE is not greater than the size of the file */
    if (startByte + BLOCKSIZE >= wanted_disk->size) {
        return ERR_RPASTLIMIT;
    }

    /* Go into file and set head of reader at the startByte */
    if (fseek(file, startByte, SEEK_SET) != 0) {
        return ERR_RSEEKISSUE;
    }

    /* Read the BLOCKSIZE into block */
    size_t bytesRead = fread(block, 1, BLOCKSIZE, file);
    if (bytesRead != BLOCKSIZE) {
        return ERR_READISSUE;
    }
    
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
