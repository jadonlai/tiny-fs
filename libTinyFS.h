#include "tinyFS.h"



#define SUPERBLOCK 1
#define INODE 2
#define FILEEXTENT 3
#define FREEBLOCK 4
#define MAGIC 0x44
#define DATASIZE 252

typedef struct ResourceTable {
    int *inodes;
    fileDescriptor *filePointers;
    char **names;
} ResourceTable;

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *diskname);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);
