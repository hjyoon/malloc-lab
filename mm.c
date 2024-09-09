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

void* extend_heap(size_t);
// void* coalesce(void*);
void* find_fit(size_t);
// void place(void*, size_t);
int get_free_list_pos(size_t);

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */
#define DSIZE 8 /* Double word size (bytes) */
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

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

#define NEXT_FREEP(bp) (*(void**)(bp))

#define LIMIT_SIZE 11

static char* heap_listp;

void remove_in_freelist(void* bpp) {
    // free_listp = NEXT_FREEP(bp);
    NEXT_FREEP(bpp) = NEXT_FREEP(NEXT_FREEP(bpp));
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    // printf("TEST\n");
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk((LIMIT_SIZE+5)*WSIZE)) == (void*)-1) {
        // printf("mem_sbrk ERROR\n");
        return -1;
    }
    // printf("%p\n", heap_listp);
    // printf("TEST2\n");
    PUT(heap_listp, 0);
    PUT(heap_listp + (WSIZE), PACK((LIMIT_SIZE+5)*WSIZE, 1)); /* Prologue header */

    PUT(heap_listp + (2*WSIZE), (unsigned int)NULL); /* 16 */
    PUT(heap_listp + (3*WSIZE), (unsigned int)NULL); /* 32 */
    PUT(heap_listp + (4*WSIZE), (unsigned int)NULL); /* 64 */
    PUT(heap_listp + (5*WSIZE), (unsigned int)NULL); /* 128 */
    PUT(heap_listp + (6*WSIZE), (unsigned int)NULL); /* 256 */
    PUT(heap_listp + (7*WSIZE), (unsigned int)NULL); /* 512 */
    PUT(heap_listp + (8*WSIZE), (unsigned int)NULL); /* 1024 */
    PUT(heap_listp + (9*WSIZE), (unsigned int)NULL); /* 2048 */
    PUT(heap_listp + (10*WSIZE), (unsigned int)NULL); /* 4096 */
    PUT(heap_listp + (11*WSIZE), (unsigned int)NULL); /* 8192 */
    PUT(heap_listp + (12*WSIZE), (unsigned int)NULL); /* 16384 */
    PUT(heap_listp + (13*WSIZE), (unsigned int)NULL); /* 32768 (INF) */

    PUT(heap_listp + (14*WSIZE), PACK((LIMIT_SIZE+5)*WSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (15*WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*WSIZE);
    // free_listp = heap_listp;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
    //     return -1;
    return 0;
}

void* extend_heap(size_t words) {
    char* bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

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
    char* bp;
    void* bpp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bpp = find_fit(asize)) != NULL) {
        // place(bp, asize);
        // printf("malloc1\n");
        // printf("%p\n", bpp);
        bp = NEXT_FREEP(bpp);
        // printf("malloc2\n");
        if (bp != NULL) {
            // printf("malloc2-1\n");
            remove_in_freelist(bpp);
            return bp;
        }
    }

    // printf("malloc3\n");

    /* No fit found. Get more memory and place the block */
    // extendsize = MAX(asize, CHUNKSIZE);
    // if ((bpp = extend_heap(extendsize/WSIZE)) == NULL)
    //     return NULL;

    extendsize = 1 << (get_free_list_pos(asize)+4);
    // printf("extendsize=%d\n", extendsize);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;

    // printf("malloc4\n");

    // place(bp, asize);
    // remove_in_freelist(bpp);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // coalesce(bp);
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

int get_free_list_pos(size_t asize) {
    int s = 16;
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

void* find_fit(size_t asize) {
    int i = get_free_list_pos(asize);
    void* bp = heap_listp+WSIZE*i;
    // printf("heap_listp=%p\n", heap_listp);
    // printf("i=%d\n", i);
    // printf("WSIZE=%d\n", WSIZE);
    // printf("bp=%p\n", bp);
    return bp;
}