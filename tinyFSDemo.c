/* TinyFS demo file
 * Foaad Khosmood, Cal Poly / modified Winter 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinyFS.h"
#include "libTinyFS.h"
#include "TinyFS_errno.h"



/* simple helper function to fill Buffer with as many inPhrase strings as possible before reaching size */
int fillBufferWithPhrase(char *inPhrase, char *Buffer, int size) {
  int index = 0, i;
  if (!inPhrase || !Buffer || size <= 0 || size < strlen(inPhrase))
    return -1;

  while (index < size) {
    for (i = 0; inPhrase[i] != '\0' && (i + index < size); i++)
	    Buffer[i + index] = inPhrase[i];
    index += i;
  }
  Buffer[size - 1] = '\0';	/* explicit null termination */
  return 0;
}

/* This program will create 2 files (of sizes 200 and 1000) to be read from or stored in the TinyFS file system. */
int main() {
  char readBuffer;
  char *afileContent, *bfileContent;	/* buffers to store file content */
  int afileSize = 30;		            /* sizes in bytes */
  int bfileSize = 500;

  char phrase1[] = "hello world from (a) file ";
  char phrase2[] = "(b) file content ";

  fileDescriptor aFD, bFD;

  char *defaultDiskName = "tinyFSDemoDisk";

  /* try to mount the disk */
  if (tfs_mount(defaultDiskName) < 0) {             /* if mount fails */
    tfs_mkfs(defaultDiskName, DEFAULT_DISK_SIZE);	  /* then make a new disk */
    if (tfs_mount(defaultDiskName) < 0) {	          /* if we still can't open it... */
	    perror("failed to open disk");	              /* then just exit */
	    return 1;
	  }
  }

  afileContent = (char *) malloc(afileSize * sizeof(char));
  if(fillBufferWithPhrase(phrase1, afileContent, afileSize) < 0) {
    perror("failed");
    return 1;
  }

  bfileContent = (char *) malloc(bfileSize * sizeof(char));
  if (fillBufferWithPhrase(phrase2, bfileContent, bfileSize) < 0) {
    perror("failed");
    return 1;
  }

  /* print content of files for debugging */
  printf("(a) File content: %s\n(b) File content: %s\nReady to store in TinyFS\n", afileContent, bfileContent);

  /* read or write files to TinyFS */
  aFD = tfs_openFile("afile");

  if (aFD < 0) {
    perror("tfs_openFile failed on afile");
  }

  printf("afile post creating\n");
  if (tfs_readFileInfo(aFD) < 0) {
    perror("tfs_readFileInfo failed");
  }
  sleep(3);

  /* now, was there already a file named "afile" that had some content? If we can read from it, yes!
   * If we can't read from it, it presumably means the file was empty.
   * If the size is 0 (all new files are sized 0) then any "readByte()" should fail, so 
   * it's a new file and empty */
  if (tfs_readByte(aFD, &readBuffer) < 0) {
    /* if readByte() fails, there was no afile, so we write to it */
    if (tfs_writeFile(aFD, afileContent, afileSize) < 0) {
	    perror("tfs_writeFile failed");
	  } else {
        printf("afile post writing\n");
        if (tfs_readFileInfo(aFD) < 0) {
          perror("tfs_readFileInfo failed");
        }
	    printf("Successfully written to afile\n");
    }
  } else {
    /* if yes, then just read and print the rest of afile that was already there */
    printf("*** reading afile from TinyFS:\n%c\n", readBuffer);  /* print the first byte already read */
    /* set afile to read only */
    printf("setting afile to readonly\n");
    if (tfs_makeRO("afile") < 0) {
        perror("tfs_makeRO failed");
    }
    /* write a new byte */
    printf("writing byte: 9\n");
    if (tfs_writeByte(aFD, 57) < 0) {
        printf("error correctly\n");
    } else {
        printf("succeeded incorrectly\n");
    }
    /* set afile to read write */
    printf("changing back to read write and writing byte: 9\n");
    if (tfs_makeRW("afile") < 0) {
        perror("tfs_makeRW failed");
    }
    if (tfs_writeByte(aFD, 57) < 0) {
        printf("error incorrectly\n");
    } else {
        printf("succeeded correctly\n");
    }
    /* move the pointer back */
    tfs_seek(aFD, 1);
    /* now print the rest of it, byte by byte */
    printf("reading again:\n");
    while (tfs_readByte(aFD, &readBuffer) >= 0)  /* go until readByte fails */
	    printf("%c", readBuffer);
    printf("\nafile post reading\n");
    if (tfs_readFileInfo(aFD) < 0) {
      perror("tfs_readFileInfo failed");
    }
    /* close file */
    if (tfs_closeFile (aFD) < 0)
	    perror("tfs_closeFile failed");

    /* now try to delete the file. It should fail because aFD is no longer valid */
    if (tfs_deleteFile (aFD) < 0) {
	    aFD = tfs_openFile("afile"); /* so we open it again */
	    if (tfs_deleteFile (aFD) < 0)
	      perror("tfs_deleteFile failed");
        else
          printf("tfs_deleteFile succeeded\n");
	  } else {
	    perror("tfs_deleteFile should have failed");
    }
  }

  /* now bfile tests */
  bFD = tfs_openFile("bfile");
  sleep(3);

  if (bFD < 0) {
    perror("tfs_openFile failed on bfile");
  }

  printf("bfile just opened\n");
  if (tfs_readFileInfo(bFD) < 0) {
    perror("tfs_readFileInfo failed");
  }

  if (tfs_readByte(bFD, &readBuffer) < 0) {
    if (tfs_writeFile(bFD, bfileContent, bfileSize) < 0) {
	    perror("tfs_writeFile failed");
	  } else {
        printf("bfile post writing\n");
        if (tfs_readFileInfo(aFD) < 0) {
          perror("tfs_readFileInfo failed");
        }
	    printf("Successfully written to bfile\n");
    }
  } else {
    printf("*** reading bfile from TinyFS:\n%c", readBuffer);
    printf("\nwriting byte: 9\n");
    tfs_writeByte(bFD, 57);
    tfs_seek(bFD, 1);
    printf("reading again:\n");
    while (tfs_readByte(bFD, &readBuffer) >= 0)
	    printf("%c", readBuffer);
    printf("\nbfile post reading\n");
    if (tfs_readFileInfo(bFD) < 0) {
      perror("tfs_readFileInfo failed");
    }
  }

  /* print the fragments before we fix them */
  printf("disk with fragmentation\n");
  tfs_displayFragments();

  /* defragment the disk */
  tfs_defrag();

  /* print the fragments */
  printf("disk with fixed fragmentation\n");
  tfs_displayFragments();

  /* Free both content buffers */
  free(bfileContent);
  free(afileContent);
  if (tfs_unmount() < 0)
    perror ("tfs_unmount failed");

  printf ("\nend of demo\n\n");
  return 0;
}
