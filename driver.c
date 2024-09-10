#include "mm.h"
#include "memlib.h"

int verbose = 0;

int main(void) {
    printf("Hello World!\n");
    mem_init();

    mm_init();
    mem_sbrk(1024);

    int size = 100;
    char* p = mm_malloc(size);


    char* lo = p;
    char *hi = lo + size - 1;

    if ((lo < (char *)mem_heap_lo()) || (lo > (char *)mem_heap_hi()) || 
	(hi < (char *)mem_heap_lo()) || (hi > (char *)mem_heap_hi())) {
	    printf("Payload (%p:%p) lies outside heap (%p:%p)\n", lo, hi, mem_heap_lo(), mem_heap_hi());
        // return 0;
    }

    printf("mem_heap_lo() = %p\n", mem_heap_lo());
    printf("mem_heap_hi() = %p\n", mem_heap_hi());
    printf("%d\n", mem_heap_hi() - mem_heap_lo());
    printf("lo = %p\n", lo);
    printf("hi = %p\n", hi);
    printf("%d\n", hi - lo);


    // mm_malloc(32);
    // mm_malloc(32);
    return 0;
}