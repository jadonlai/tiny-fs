#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define OPEN 1
#define CLOSED 1

typedef struct Disk {
    int diskNumber;
    int diskSize;
    int status;
    char *fileName;
    FILE *file;
    struct Disk *next;
} Disk;

int closeDisk(int diskNumber);
int changeDiskStatusFileName(char* filename, int status);
int changeDiskStatusNumber(int diskNumber, int status);
int updateDiskFile(FILE* file, int diskNumber);
Disk *findDiskNodeNumber(int diskNumber);
Disk *findDiskNodeFileName(char *filename);
int addDiskNode(int diskNumber, int diskSize, char *filename, FILE* file);
int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);
