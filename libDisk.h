#include <stdio.h>
#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define OPEN 1
#define CLOSED 0

typedef struct Disk {
    int diskNumber;
    int diskSize;
    int status;
    char *fileName;
    FILE *file;
    struct Disk *next;
} Disk;

extern int closeDisk(int diskNumber);
extern int changeDiskStatusFileName(char* filename, int status);
extern int changeDiskStatusNumber(int diskNumber, int status);
extern int updateDiskFile(FILE* file, int diskNumber);
extern Disk *findDiskNodeNumber(int diskNumber);
extern Disk *findDiskNodeFileName(char *filename);
extern int addDiskNode(int diskNumber, int diskSize, char *filename, FILE* file);
extern int openDisk(char *filename, int nBytes);
extern int closeDisk(int disk);
extern int readBlock(int disk, int bNum, void *block);
extern int writeBlock(int disk, int bNum, void *block);
