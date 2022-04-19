#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/ioc_homework.h>

#define HOMEWORK_DEVICE "/dev/homework"

int main()
{
	int ret, slot_index, ret_index;
	int fd = open(HOMEWORK_DEVICE, O_RDWR);

	if(fd < 0) {
            perror("open");
	    exit(EXIT_FAILURE);
	}

	slot_index = 4; /* default selected */
	/* HIOCSLOT */
	if(ioctl(fd, HIOCSLOT, &slot_index) < 0) {
	    printf("%s\n", strerror(errno));
	    exit(EXIT_FAILURE);
	}

	/* get the current slot */
	ret_index = 5;
	if(ioctl(fd, HIOCGETSLOT, &ret_index) < 0) {
	    perror("HIOCGETSLOT");
	    return EXIT_FAILURE;
	}
	printf("current slot index: %d\n", ret_index);

	/* clear the selected slot  */
	if(ioctl(fd, HIOCCLEARSLOT) < 0) {
	    perror("HIOCCLEARSLOT");
	    return EXIT_FAILURE;
	}

	/* expect failure when attempting to read cleared slot */
	if(ioctl(fd, HIOCGETSLOT, &ret_index) < 0) {
	    printf("[success] some other failure\n");
	} else {
	    printf("err as expected\n");
	    ret = close(fd);
	}
	
	ret = close(fd);

	return ret;
}
