#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

/* opens regular UNIX File */
int openDisk(char *filename, int nBytes) {
    FILE* file;
    int diskNumber = diskCount;

    /* Opens file + designates first nBytes as space for emulated disk */
    if (nBytes == 0) {    
        /* Opens existing file and its contents may not be overwritten */
        file = fopen(filename, "r"); 

        /* Find the disk entry in list with filename and set it to open */
        diskNumber = changeDiskStatusFileName(filename, OPEN);
        if (diskNumber < 0) {   /* if error */
            return ERR_FINDANDCHANGESTATUS;
        }

        /* update the FILE pointer in our node */
        if (updateDiskFile(file, diskNumber) < 0) {
            return ERR_CANNOTFNDDISK;
        }   
    }
    else if(nBytes < BLOCKSIZE) { /* Issue with nBytes, Return error code */
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

        /* Add new disk to linked list */
        if(addDiskNode(diskNumber, amount, filename, file)) {
            return ERR_ADDDISK;
        }
        diskCount++;    /* incrementing diskCount for next disk */
    }

    if (file == NULL) {    /* confirming that the file actually was opened */
        return ERR_NOFILE; 
    }

    /* Return disk number of disk we just were dealing with*/
    return diskNumber;
}


/* Add new disk node to linked list at the end. Return negative value if issue else 0 if success */
int addDiskNode(int diskNumber, int diskSize, char *filename, FILE* file) {
    /* make disk Node for new disk */ 
    Disk *new_disk = (Disk *)malloc(sizeof(Disk));
    if (new_disk == NULL) {
        return ERR_ADDDISK;
        /* Q: Where should we free? */
    }
    new_disk->diskNumber = diskNumber; 
    new_disk->diskSize = diskSize; 
    new_disk->status = 1; /* open status */
    new_disk->filename = filename;
    new_disk->file = file;
    new_disk->next = NULL;

    /* if list empty add to front */
    if (head == NULL) {
        head = new_disk;
        return 0;
    }
    Disk *temp = head;

    /* loop through linked list until get to end */
    while(temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = new_disk;
    return 0;
}

/* Find the DiskNode based on diskNumber in our Linked List*/
Disk *findDiskNode(int diskNumber) {
    Disk *temp = head;
    while(temp) {
        if (temp->diskNumber == diskNumber) {
            return temp;
        }
        else {
            temp = temp->next;
        }
    }   
    return NULL;
}


int updateDiskFile(FILE* file, int diskNumber) {
    Disk *temp = findDiskNode(diskNumber);
    if(temp == NULL) {
        return ERR_FINDANDCHANGESTATUS;
    } 
    else {
        temp->file = file;
        return 0;
    }   
}

/* Go through Disk and find disk based on diskNumber. Change status to Open or Closed. 
    Return 0 on success and error on failure. */
int changeDiskStatusNumber(int diskNumber, int status) {
    Disk *temp = findDiskNode(diskNumber);
    if(temp == NULL) {
        return ERR_FINDANDCHANGESTATUS;
    } 
    else {
        temp->status = status;
        return 0;
    }
    // Disk *temp = head;
    // while(temp) {
    //     if (temp->diskNumber == diskNumber) {
    //         temp->status = status;
    //         return 0;
    //     }
    //     else {
    //         temp = temp->next;
    //     }
    // }  
}

/* Go through Disk and find disk based on filename. Change status to Open or Closed. 
    Return diskNumber on success or negative value on failure. */
int changeDiskStatusFileName(char* filename, int status) {
    Disk *temp = head;
    while(temp) {
        if (strcmp(temp->filename, filename) == 0) {
            temp->status = status;
            return temp->diskNumber;
        }
        else {
            temp = temp->next;
        }
    }  
    return ERR_FINDANDCHANGESTATUS;
}

int closeDisk(int diskNumber) {
    /* Find wanted disk */
    Disk* wanted_disk = findDiskNode(diskNumber); /* Go into our linked list structure, find diskNode matching diskNum*/
    if(wanted_disk == NULL) {
        return ERR_CANNOTFNDDISK;
    }

    /* close open file */
    FILE *file = wanted_disk->file;
    if (file == NULL) {
        return ERR_FILEISSUE;
    }
    wanted_disk->file = NULL;
    fclose(file);   /* find file and close it */

    changeDiskStatusNumber(diskNumber, CLOSED); /* close disk in linkedlist */
    return 0;
}

int readBlock(int disk, int bNum, void *block) {
    Disk* wanted_disk = findDiskNode(disk); /* Go into our linked list structure, find diskNode matching diskNum*/
    if(wanted_disk == NULL) {
        return ERR_CANNOTFNDDISK;
    }

    FILE *file = wanted_disk->file;
    if (file == NULL) {
        return ERR_FILEISSUE;
    }

    /* Check that block is the size of a BLOCKSIZE  */ 
    if ((block != NULL && sizeof(block) >= 256) || block == NULL) {
    return ERR_RBLOCKISSUE;
    }

    int startByte = bNum * BLOCKSIZE;

    /*  Check that startByte + BLOCKSIZE is not greater than the size of the file */
    if (startByte + BLOCKSIZE >= wanted_disk->diskSize) {
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
    Disk *wanted_disk = findDiskNode(disk); /* Go into our linked list structure, find diskNode matching diskNum*/
    if(wanted_disk == NULL) {
        return ERR_CANNOTFNDDISK;
    }
    
    FILE *file = wanted_disk->file;
    if (file == NULL) {
        return ERR_FILEISSUE;
    }

    /* Check that block is the size of a BLOCKSIZE */ 
    if ((block != NULL && sizeof(block) >= 256) || block == NULL) {
        return ERR_RBLOCKISSUE;
    }

    int startByte = bNum * BLOCKSIZE;
    /*  Check that startByte + BLOCKSIZE is not greater than the size of the file */
    if (startByte + BLOCKSIZE >= wanted_disk->diskSize) {
        return ERR_RPASTLIMIT;
    }
    
    // writes to file 
    if (fseek(file, startByte, SEEK_SET) != 0) {    /* moves head of file to startByte */
        return ERR_WSEEKISSUE;
    }

    size_t bytesWritten = fwrite(block, 1, BLOCKSIZE, file);
    if (bytesWritten != BLOCKSIZE) {
        return ERR_WRITEISSUE;
    }

    return 0;
}
