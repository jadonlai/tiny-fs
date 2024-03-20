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
#define NAMELENGTH 9
#define TIMELENGTH 11
#define SIZELENGTH 6
#define MAXTIMESTRING 26

typedef struct FileDetails {
    int inode;
    char *name;
    fileDescriptor fd;
    int filePointer;
    int rw;
} FileDetails;

/* Inode Block:
 * 0: Block Type
 * 1: 0x44
 * 2: Link
 * 3: Empty
 * 4-12: Name
 * 13-18: Size
 * 19-29: Creation Time
 * 30-40: Modification Time
 * 41-51: Access Time
 * 
 * Note: The name, size, creation, modification, and access time end in null bytes */

extern int tfs_mkfs(char *filename, int nBytes);
extern int tfs_mount(char *diskname);
extern int tfs_unmount(void);
extern fileDescriptor tfs_openFile(char *name);
extern int tfs_closeFile(fileDescriptor FD);
extern int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
extern int tfs_deleteFile(fileDescriptor FD);
extern int tfs_readByte(fileDescriptor FD, char *buffer);
extern int tfs_writeByte(fileDescriptor FD, unsigned int data);
extern int tfs_seek(fileDescriptor FD, int offset);
extern void tfs_displayFragments();
extern int tfs_defrag();
extern int tfs_makeRO(char *name);
extern int tfs_makeRW(char *name);
extern int tfs_readFileInfo(fileDescriptor FD);
