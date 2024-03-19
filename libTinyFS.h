#include "tinyFS.h"
#include <time.h>


#define SUPERBLOCK 1
#define INODE 2
#define FILEEXTENT 3
#define FREEBLOCK 4
#define MAGIC 0x44
#define DATASIZE 252
#define READ 1
#define WRITE 2
#define READWRITE 3
#define NUM_BLOCKS 40
#define MAXNAMECHARS 8
#define MAXTIMESTRING 26

typedef struct FileDetails {
    int inode;
    char *name;
    fileDescriptor fd;
    int filePointer;
    time_t creationTime; 
    time_t accessTime; 
    time_t modificationTime;
} FileDetails;

extern int tfs_mkfs(char *filename, int nBytes);
extern int tfs_mount(char *diskname);
extern int tfs_unmount(void);
extern fileDescriptor tfs_openFile(char *name);
extern int tfs_closeFile(fileDescriptor FD);
extern int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
extern int tfs_deleteFile(fileDescriptor FD);
extern int tfs_readByte(fileDescriptor FD, char *buffer);
extern int tfs_seek(fileDescriptor FD, int offset);
extern void tfs_displayFragments();
extern time_t tfs_getCreationTime(fileDescriptor FD);
extern time_t getFileAccessTime(fileDescriptor FD);
extern time_t getFileModificationTime(fileDescriptor FD);
extern int updateModificationTime(fileDescriptor FD);
extern int updateAccessTime(fileDescriptor FD);
void printTimes(fileDescriptor FD);

int tfs_rename(fileDescriptor FD, char* newName);
int tfs_readdir();