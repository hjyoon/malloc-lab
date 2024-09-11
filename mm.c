/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "6h057.sh",
    /* First member's full name */
    "Hyojeon Yoon",
    /* First member's email address */
    "hjyoon@kakao.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

static void* extend_heap(size_t);
static void* coalesce(void*);
static void* find_fit(size_t);
static void place(void*, size_t);

#define LIMIT_SIZE 24

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<LIMIT_SIZE) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*) (p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

// #define PRED(bp) ((unsigned int*)(bp))
#define SUCC(bp) ((unsigned int*)(bp) + WSIZE)

// #define PREV_FREEP(bp) (*(unsigned int*)(bp))
// #define NEXT_FREEP(bp) (*(unsigned int*)(bp + WSIZE))

#define PREV_FREEP(bp) (*(void**)(bp))
#define NEXT_FREEP(bp) (*(void**)(bp + WSIZE))

static char* heap_listp;
static char* free_listp;

void remove_in_freelist(void* bp) {
    if (bp == free_listp) {
        PREV_FREEP(NEXT_FREEP(bp)) = NULL;
        free_listp = NEXT_FREEP(bp);
    } else {
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
        PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
    }
}

void put_front_of_freelist(void* bp) {
    NEXT_FREEP(bp) = free_listp;
    PREV_FREEP(bp) = NULL;
    PREV_FREEP(free_listp) = bp;
    free_listp = bp;
}

int get_free_list_pos(size_t asize) {
    int s = 1;
    int cnt = 0;
    while(cnt < LIMIT_SIZE) {
        if(s >= asize) {
            break;
        }
        s *= 2;
        cnt += 1;
    }
    return cnt;
}

int init_freed_block(char* p, size_t size) {
    p += WSIZE;
    PUT(HDRP(p), PACK(size, 0));
    PUT(FTRP(p), PACK(size, 0));
    return 0;
}

int split_chunk(void* p) {
    size_t size = GET_SIZE(p);

    // printf("%p %u\n", p, size/2);
    // printf("%p %u\n", p+size/2, size/2);

    init_freed_block(p, size/2);
    init_freed_block(p+size/2, size/2);

    int i = get_free_list_pos(size/2);
    *(unsigned int**)(heap_listp+i*WSIZE) = (unsigned int*)p;
    printf("%d\n", i);

    // printf("%u\n", size);

    return size/2;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    /* Create the initial empty heap */
    int init_block_size = (LIMIT_SIZE+4)*WSIZE;

    if ((heap_listp = mem_sbrk(init_block_size)) == (void*)-1) {
        printf("mem_sbrk returns -1\n");
        return -1;
    }
    // printf("heap_listp: %p (%u)\n", heap_listp, (unsigned int)heap_listp);
    // printf("mem_heap_lo: %p (%u)\n", mem_heap_lo(), (unsigned int)mem_heap_lo());
    // printf("mem_heap_hi: %p (%u)\n", mem_heap_hi(), (unsigned int)mem_heap_hi());
    // printf("mem_heapsize: %p (%u)\n", mem_heapsize(), (unsigned int)mem_heapsize());

    int i = 0;

    PUT(heap_listp + ((i++)*WSIZE), 0); /* Alignment padding */
    PUT(heap_listp + ((i++)*WSIZE), PACK(init_block_size, 1)); /* Prologue header */

    for (; i<LIMIT_SIZE+2; i++) {
        PUT(heap_listp + (i*WSIZE), (unsigned int)NULL);
    }

    PUT(heap_listp + ((i++)*WSIZE), PACK(init_block_size, 1)); /* Prologue footer */
    PUT(heap_listp + ((i++)*WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*WSIZE);
    free_listp = heap_listp;

    // printf("heap_listp: %p (%u)\n", heap_listp, (unsigned int)heap_listp);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    void* bp = extend_heap(CHUNKSIZE/WSIZE);
    if (bp == NULL) {
        printf("extend_heap returns NULL\n");
        return -1;
    }

    int j = get_free_list_pos(CHUNKSIZE);

    PUT(bp, PACK(CHUNKSIZE, 0));
    PUT(bp+WSIZE, (unsigned int)NULL);

    *(unsigned int**)(heap_listp+j*WSIZE) = (unsigned int*)bp;

    // printf("%u\n", CHUNKSIZE);
    // printf("%u\n", GET_SIZE(*(unsigned int*)(heap_listp+j*WSIZE)));

    return 0;
}

static void* extend_heap(size_t words) {
    char* bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // printf("bp: %p (%u)\n", bp, (unsigned int)bp);
    // printf("mem_heap_lo: %p (%u)\n", mem_heap_lo(), (unsigned int)mem_heap_lo());
    // printf("mem_heap_hi: %p (%u)\n", mem_heap_hi(), (unsigned int)mem_heap_hi());
    // printf("mem_heapsize: %p (%u)\n", mem_heapsize(), (unsigned int)mem_heapsize());

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    // printf("%p (%u)\n", *(unsigned int*)HDRP(bp), *(unsigned int*)HDRP(bp));
    // printf("%p (%u)\n", *(unsigned int*)HDRP(NEXT_BLKP(bp)), *(unsigned int*)HDRP(NEXT_BLKP(bp)));
    
    // printf("%p (%u)\n", PREV_BLKP(bp), PREV_BLKP(bp));
    // printf("%p (%u)\n", GET_ALLOC(HDRP(NEXT_BLKP(bp))), GET_ALLOC(HDRP(NEXT_BLKP(bp))));

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void* coalesce(void* bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // printf("bp: %p (%u)\n", bp, (unsigned int)bp);
    // printf("prev_alloc: %d\n", prev_alloc);
    // printf("next_alloc: %d\n", next_alloc);

    if (prev_alloc && next_alloc) { /* Case 1 */
        return bp;
    } else if (prev_alloc && !next_alloc) { /* Case 2 */
        remove_in_freelist(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        /* explicit custom */
        // PUT(SUCC(bp), GET(SUCC(NEXT_BLKP(bp))));
        // PUT(PRED(NEXT_BLKP(NEXT_BLKP(bp))), (unsigned int)bp);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { /* Case 3 */
        remove_in_freelist(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        /* explicit custom */
        // PUT(SUCC(PREV_BLKP(bp)), GET(SUCC(bp)));
        // PUT(PRED(NEXT_BLKP(bp)), GET(PRED(bp)));

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        remove_in_freelist(PREV_BLKP(bp));
        remove_in_freelist(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        // unsigned int prev_blkp = (unsigned int)PREV_BLKP(PREV_BLKP(bp));
        // unsigned int next_blkp = (unsigned int)NEXT_BLKP(NEXT_BLKP(bp));

        /* explicit custom */
        // PUT(SUCC(prev_blkp), next_blkp);
        // PUT(PRED(next_blkp), prev_blkp);

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    put_front_of_freelist(bp);
    return bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char* p;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    // if ((bp = find_fit(asize)) != NULL) {
    //     place(bp, asize);
    //     return bp;
    // }

    p = find_fit(asize);
    // printf("%p\n", *(unsigned int*)bp);
    int suitable_size = *(unsigned int*)p;
    // printf("%d\n", suitable_size);
    while (1) {
        suitable_size = split_chunk(p);
        if (suitable_size/2 < asize) {
            break;
        }
        suitable_size /= 2;
    }

    printf("%d\n", suitable_size);

    // if(p) {
    //     return p;
    // }


    /* No fit found. Get more memory and place the block */
    // extendsize = MAX(asize, CHUNKSIZE);
    // if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
    //     return NULL;
    place(p, asize);
    return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size)
{
    void* oldptr = ptr;
    void* newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL) {
        return NULL;
    }
    // copySize = *(size_t*)((char*)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(ptr));

    if (size < copySize) {
        copySize = size;
    }
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

// static void* find_fit(size_t asize) {
//     /* First-fit search */
//     void* bp = free_listp;

//     while(bp) {
//         if(asize <= GET_SIZE(HDRP(bp))) {
//             return bp;
//         } else {
//             bp = NEXT_FREEP(bp);
//         }
//     }

//     return NULL; /* No fit */
// }

void* find_fit(size_t asize) {
    int i = get_free_list_pos(asize);
    void* p = heap_listp+WSIZE*i;
    while(!*(unsigned int*)p) {
        i++;
        p = heap_listp+WSIZE*i;
    }
    // printf("%u\n", GET_SIZE(*(unsigned int*)p));
    return *(void**)p;
}

static void place(void* p, size_t asize) {
    void* bp = p + WSIZE;
    size_t csize = GET_SIZE(HDRP(bp));

    // printf("%d\n", csize);

    // if ((csize - asize) >= (2*DSIZE)) {
    //     // unsigned int prev_blkp = (unsigned int)PREV_BLKP(bp);
    //     // unsigned int next_blkp = (unsigned int)NEXT_BLKP(bp);

    //     PUT(HDRP(bp), PACK(asize, 1));
    //     PUT(FTRP(bp), PACK(asize, 1));
    //     bp = NEXT_BLKP(bp);

    //     PUT(HDRP(bp), PACK(csize-asize, 0));
    //     PUT(FTRP(bp), PACK(csize-asize, 0));
    // } else {
    //     PUT(HDRP(bp), PACK(csize, 1));
    //     PUT(FTRP(bp), PACK(csize, 1));
    // }

    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
}
