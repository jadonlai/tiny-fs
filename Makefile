CC = gcc
CFLAGS = -Wall -g
PROG = tinyFSDemo
OBJS = tinyFSDemo.o libTinyFS.o libDisk.o
TESTS = diskTest tfsTest

all: tinyFSDemo diskTest tfsTest

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

tinyFSDemo.o: tinyFSDemo.c libTinyFS.h tinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c libTinyFS.h tinyFS.h libDisk.h libDisk.o TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

diskTest.o: diskTest.c libDisk.h
	$(CC) $(CFLAGS) -c -o $@ $<

diskTest: diskTest.o libDisk.o
	$(CC) $(CFLAGS) -o diskTest diskTest.o libDisk.o

tfsTest.o: tfsTest.c tinyFS.h libTinyFS.h TinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

tfsTest: tfsTest.o libTinyFS.o libDisk.o
	$(CC) $(CFLAGS) -o tfsTest tfsTest.o libTinyFS.o libDisk.o

clean:
	rm libDisk.o libTinyFS.o diskTest.o tfsTest.o tinyFSDemo.o diskTest tfsTest tinyFSDemo disk0.dsk disk1.dsk disk2.dsk disk3.dsk tinyFSDisk
