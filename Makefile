CC = gcc
CFLAGS = -Wall -g $(shell pkg-config fuse --cflags) -std=gnu11
LDLIBS = $(shell pkg-config fuse --libs)

EXT2_IMPL_OBJECTS = ext2.o ext2symlink.o ext2dir.o ext2file.o

all: ext2fs ext2test

ext2fs: ext2fs.o $(EXT2_IMPL_OBJECTS)
ext2test: ext2test.o $(EXT2_IMPL_OBJECTS)

clean:
	-rm -rf ext2fs ext2test ext2fs.o ext2test.o $(EXT2_IMPL_OBJECTS)
tidy: clean
	-rm -rf *~
