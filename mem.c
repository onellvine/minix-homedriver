#include <stdlib.h>
#include <stdio.h>


/* variable in global scope */
int global_var;


/* this function will be loaded into the stack */
void *defunc()
{
    int data = 404;
    return &data;
}


int main(int argc, const char **argv)
{
    /* declare pointer memory addresses */
    void *stack_addr, *data_addr, *heap_addr;

    /* function pointer for stack text address */
    void *(*text_addr)();

    /* assignments */
    text_addr = &defunc;
    stack_addr = defunc();
    data_addr = &global_var;
    heap_addr = malloc(sizeof(int));

    printf("Text Address %p\nStack Address %p\nGlobal Data Address %p\nHeap Data Address %p\n", text_addr, stack_addr, data_addr, heap_addr);

    free(heap_addr);
	
    return 0;
}


