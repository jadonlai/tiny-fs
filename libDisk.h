#include "tinyFS.h"
#include "<stdio.h>"

int diskCount;
typedef struct Disk {
    int diskNumber;
    int diskSize;
    int status;
    char *filename;
    struct Disk *next;
} Disk;

int openDisk(char *filename, int nBytes);
int closeDisk(int disk);
int readBlock(int disk, int bNum, void *block);
int writeBlock(int disk, int bNum, void *block);

