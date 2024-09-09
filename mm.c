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

// #define PRED(bp) ((unsigned int*)(bp))
// #define SUCC(bp) ((unsigned int*)(bp) + WSIZE)

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

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(6*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(2*DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), (unsigned int)NULL);
    PUT(heap_listp + (3*WSIZE), (unsigned int)NULL);
    PUT(heap_listp + (4*WSIZE), PACK(2*DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (5*WSIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*WSIZE);
    free_listp = heap_listp;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

static void* extend_heap(size_t words) {
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

    /* explicit custom */
    // PUT(PRED(bp), (unsigned int)free_list_tail);
    // PUT(SUCC(bp), (unsigned int)NULL);
    // free_list_tail = (unsigned int*)bp;

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void* coalesce(void* bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        // return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        remove_in_freelist(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        /* explicit custom */
        // PUT(SUCC(bp), GET(SUCC(NEXT_BLKP(bp))));
        // PUT(PRED(NEXT_BLKP(NEXT_BLKP(bp))), (unsigned int)bp);

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        remove_in_freelist(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        /* explicit custom */
        // PUT(SUCC(PREV_BLKP(bp)), GET(SUCC(bp)));
        // PUT(PRED(NEXT_BLKP(bp)), GET(PRED(bp)));

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {
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
    char* bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
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

static void* find_fit(size_t asize) {
    /* First-fit search */
    void* bp = free_listp;

    while(bp) {
        if(asize <= GET_SIZE(HDRP(bp))) {
            return bp;
        } else {
            bp = NEXT_FREEP(bp);
        }
    }

    return NULL; /* No fit */
}

static void place(void* bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    remove_in_freelist(bp);
    if ((csize - asize) >= (2*DSIZE)) {
        // unsigned int prev_blkp = (unsigned int)PREV_BLKP(bp);
        // unsigned int next_blkp = (unsigned int)NEXT_BLKP(bp);

        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);

        /* explicit custom */
        // PUT(SUCC(prev_blkp), (unsigned int)bp);
        // PUT(SUCC(bp), next_blkp);
        // PUT(PRED(next_blkp), (unsigned int)bp);
        // PUT(PRED(bp), prev_blkp);

        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        put_front_of_freelist(bp);
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
