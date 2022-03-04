CC = gcc
CFLAGS = -g -Wall -Wvla -Werror -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse
LDFLAGS = -lfuse -pthread

EXES = swatfs.c swatfs_mkfs.c
DEPS = $(wildcard *.h)
SOURCES = $(filter-out $(EXES), $(wildcard *.c))
OBJECTS = $(SOURCES:.c=.o)

all: swatfs swatfs_mkfs

swatfs: swatfs.c $(OBJECTS) $(DEPS)
	$(CC) $(CFLAGS) $(OBJECTS) swatfs.c -o swatfs $(LDFLAGS)

swatfs_mkfs: swatfs_mkfs.c $(DEPS)
	$(CC) $(CFLAGS) swatfs_mkfs.c -o swatfs_mkfs

%.o: %.c $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) swatfs swatfs_mkfs
