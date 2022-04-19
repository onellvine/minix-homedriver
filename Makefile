CC = clang
CFLAGS := -g -Wall

all: read write ioctl

read: test_read.c
	$(CC) $(CFLAGS) test_read.c -o $@
write: test_write.c
	$(CC) $(CFLAGS) test_write.c -o $@
ioctl: test_ioctl.c
	$(CC) $(CFLAGS) test_ioctl.c -o $@
clean:
	$(RM) read write ioctl
