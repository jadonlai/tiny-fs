CC = gcc
CFLAGS = -Wall -g
PROG = tinyFSDemo
OBJS = tinyFSDemo.o libTinyFS.o libDisk.o
TESTS = diskTest tfsTest

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

tinyFsDemo.o: tinyFSDemo.c libTinyFS.h tinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c libTinyFS.h tinyFS.h libDisk.h libDisk.o TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

diskTest: diskTest.c libDisk.h libDisk.c TinyFS_errno.h tinyFS.h
	$(CC) $(CFLAGS) diskTest.c libDisk.h libDisk.c TinyFS_errno.h tinyFS.h

tfsTest: tfsTest.c libTinyFS.h libTinyFS.c TinyFS_errno.h tinyFS.h libDisk.h libDisk.c
	$(CC) $(CFLAGS) tfsTest.c libTinyFS.h libTinyFS.c TinyFS_errno.h tinyFS.h libDisk.h libDisk.c
