#include "libDisk.h"
#include "tinyFS.h"
#include "TinyFS_errno.h"

Disk *head = NULL;
int diskCount;

/* THIS SHOULD BE DELETED */
void printDiskList(Disk *head) {
    Disk *current = head;

    printf("Disk List:\n");
    printf("----------------------------------------------------------------\n");
    printf("| Disk Number | Disk Size | Status | Filename       | File Pointer |\n");
    printf("----------------------------------------------------------------\n");

    while (current != NULL) {
        printf("| %-12d| %-10d| %-7d| %-15s| %-13p|\n", 
               current->diskNumber, current->diskSize, current->status, 
               current->fileName, current->file);
        current = current->next;
    }

    printf("----------------------------------------------------------------\n");
}

/* opens regular UNIX File */
int openDisk(char *filename, int nBytes) {
    FILE* file;
    int diskNumber = diskCount;

    /* Opens file + designates first nBytes as space for emulated disk */
    if (nBytes == 0) {    
        /* Opens existing file and its contents may not be overwritten */
        file = fopen(filename, "r"); 
        
        if (file == NULL) {    /* confirming that the file actually was opened */
            return ERR_NOFILE; 
        }

        /* Check if entry exists and if it does not we may have to make a new one */
        Disk *chosen_disk = findDiskNodeFileName(filename);

        /* If filename does not have an associated disk means that we have disks set up but are not in our linked list */
        if (chosen_disk == NULL) {

            /* getting size of file */
            fseek(file, 0, SEEK_END);   /* seeking to end of file*/
            int size = (int) ftell(file);
            int diskSize = ((size / BLOCKSIZE) + 1) * BLOCKSIZE; 
            fseek(file, 0, SEEK_SET);   /* seek to the front */

            if(addDiskNode(diskNumber, diskSize, filename, file)) {
                return ERR_ADDDISK;
            }
            
            diskCount = diskCount + 1;  /* incrementing diskCount for next disk */
        }
        else {  /* if disk entry exists then just update it */

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
    }
    else if(nBytes < BLOCKSIZE) { 
        /* Issue with nBytes, Return error code */
        return ERR_NOFILE;
    } else {
        /* Already file given by filename, the file's content may be overwritten */
        file = fopen(filename, "w+");

        if (file == NULL) {    /* confirming that the file actually was opened */
            return ERR_NOFILE; 
        }

        int amount = nBytes;
        if (nBytes % BLOCKSIZE != 0) {
            // THIS IS THE SAME AS amount = nBytes
            /* if nbytes not multiple of blocksize then set it to the closest multiple */
            amount = nBytes / BLOCKSIZE * BLOCKSIZE;
        } 

        /* Add new disk to linked list */
        if(addDiskNode(diskNumber, amount, filename, file)) {
            return ERR_ADDDISK;
        }

        diskCount = diskCount + 1;    /* incrementing diskCount for next disk */
    }

    /* Return disk number of disk we just were dealing with*/
    return diskNumber;
}

/* Add new disk node to linked list at the end. Return negative value if issue else 0 if success */
int addDiskNode(int diskNum, int diskSize, char *filename, FILE* file) {

    /* make disk Node for new disk */ 
    Disk *new_disk = (Disk *) malloc(sizeof(Disk));
    if (new_disk == NULL) {
        return ERR_ADDDISK;
    }

    new_disk->diskNumber = diskNum; 
    new_disk->diskSize = diskSize; 
    new_disk->status = OPEN; /* open status */

    /* copy string into node */
    new_disk->fileName = (char *) malloc((strlen(filename) + 1) * sizeof(char));
    if (new_disk->fileName == NULL) {
        free(new_disk); 
        return ERR_ADDDISK;
    }
    strcpy(new_disk->fileName, filename);   
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

/* Find the DiskNode based on the filename in our Linked List*/
Disk *findDiskNodeFileName(char* filename) {
    Disk *temp = head;
    while(temp) {
        if (strcmp(temp->fileName, filename) == 0) {
            return temp;
        }
        else {
            temp = temp->next;
        }
    }  
    return NULL;
}


/* Find the DiskNode based on diskNumber in our Linked List*/
Disk *findDiskNodeNumber(int diskNumber) {
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
    Disk *temp = findDiskNodeNumber(diskNumber);
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
    Disk *temp = findDiskNodeNumber(diskNumber);
    if(temp == NULL) {
        return ERR_FINDANDCHANGESTATUS;
    } 
    else {
        temp->status = status;
        return 0;
    }
}

/* Go through Disk and find disk based on filename. Change status to Open or Closed. 
    Return diskNumber on success or negative value on failure. */
int changeDiskStatusFileName(char* filename, int status) {
    Disk *temp = findDiskNodeFileName(filename);
    if(temp == NULL) {
        return ERR_FINDANDCHANGESTATUS;
    } 
    else {
        temp->status = status;
        return temp->diskNumber;
    }
}

int closeDisk(int diskNumber) {
    /* Find wanted disk */
    Disk* wanted_disk = findDiskNodeNumber(diskNumber); /* Go into our linked list structure, find diskNode matching diskNum*/
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
    Disk* wanted_disk = findDiskNodeNumber(disk); /* Go into our linked list structure, find diskNode matching diskNum*/
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
        return ERR_FINDANDCHANGESTATUS;
    }

    /* Read the BLOCKSIZE into block */
    size_t bytesRead = fread(block, 1, BLOCKSIZE, file);
    if (bytesRead != BLOCKSIZE) {
        return ERR_READISSUE;
    }

    return 0; 
}

int writeBlock(int disk, int bNum, void *block) {
    Disk *wanted_disk = findDiskNodeNumber(disk); /* Go into our linked list structure, find diskNode matching diskNum*/
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
    if (startByte + BLOCKSIZE > wanted_disk->diskSize) {
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
