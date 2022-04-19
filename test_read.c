#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define HOMEWORK_DEVICE "/dev/homework"

int main()
{
	int data;
	int fd = open(HOMEWORK_DEVICE, O_RDWR);

	if(fd < 0) {
            perror("open");
	    exit(EXIT_FAILURE);
	}

	if(read(fd, &data, sizeof(data)) < 0) {
	    printf("%s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}
	printf("read integer: %d\n", data);
	close(fd);

	return EXIT_SUCCESS;
}
