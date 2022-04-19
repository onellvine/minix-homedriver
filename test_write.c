#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define HOMEWORK_DEVICE "/dev/homework"

int main()
{
	int data = 404;
	int fd = open(HOMEWORK_DEVICE, O_RDWR);

	if(fd < 0) {
		perror("open");
		exit (EXIT_FAILURE); 
	}
	if(write(fd, &data, sizeof(data)) < 0) {
		printf("%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("written integer: %d\n", data);
	close(fd);

	return EXIT_SUCCESS;
}
