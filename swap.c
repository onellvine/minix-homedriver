#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MB 1024000L

int main()
{
    /* attempt to allocate 1 megabyte a thousand times */
    int i = 1;
    int mem_size;
    int *ptr;
    while (i < 10000)
    {
        mem_size = MB * i;
	ptr = malloc(mem_size);
	if(ptr == NULL)
	{
	    printf("failed after about %d mallocs of 1MB each \n", i);
	    exit(EXIT_FAILURE);
	}
	memset(ptr, 1, mem_size);
	i = i * 10;
    }
    free(ptr);
    exit(EXIT_SUCCESS);
}
