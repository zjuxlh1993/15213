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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#define MAX(x, y) ((x) > (y)? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define WSIZE 4// word = 4 bytes
#define DSIZE 8// 2 * word = 8 bytes
#define GET(p) (*(unsigned int *)p)
#define PUT(p, val) (*(unsigned int*)p = (val))
#define GET_SIZE(p) (GET(p) & ~0X7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))
#define CHUNKSIZE (1<<12)

void* heap_list;
int mm_init(void);
bool mm_check();
void * mm_malloc(size_t);
void mm_free(void *);
void *find_fit(size_t);
void *extend_heap(size_t);
void *place(void*, size_t);
void *mm_realloc(void *, size_t);
void coalesce(void *);

/*
*/
void *extend_heap(size_t size)
{
    void* ret;
    if (size%2 == 1) ++size;
    size_t extend_size = MAX(size, CHUNKSIZE);
    ret = mem_sbrk(extend_size * WSIZE);
    return coalesce(ret);
}
/*
*/
void coalesce(void* p)
{
    void* prev = PREV_BLKP(p);
    void* next = NEXT_BLKP(p);
    int prev_alloc_flag = GET_ALLOC(HDRP(prev));
    int next_alloc_flag = GET_ALLOC(HDRP(NEXT));
    if (prev_alloc_flag && !next_alloc_flag){
        size_t size = GET_SIZE(HDRP(p)) + GET_SIZE(HDRP(prev));
        PUT(HDRP(p), PACK(0, 0));
        PUT(FTRP(prev), PACK(0, 0));
        PUT(HDRP(prev), PACK(size, 0));
        PUT(FTRP(prev), PACK(size, 0));
        return prev;
    } else
    if (!prev_alloc_flag && next_alloc_flag){
        size_t size = GET_SIZE(HDRP(p)) + GET_SIZE(HDRP(next));
        PUT(FTRP(p), PACK(0, 0));
        PUT(HDRP(p), PACK(size, 0));
        PUT(FTRP(next), PACK(size, 0));
        PUT(HDRP(next), PACK(0, 0));
        return p;
    } else
    if (prev_alloc_flag && next_alloc_flag){
        size_t size = GET_SIZE(HDRP(p)) + GET_SIZE(HDRP(prev)) + GET_SIZE(HDRP(next));
        PUT(FTRP(prev), PACK(0, 0));
        PUT(HDRP(prev), PACK(size, 0));
        PUT(FTRP(p), PACK(0, 0));
        PUT(HDRP(p), PACK(0, 0));
        PUT(FTRP(next), PACK(size, 0));
        PUT(HDRP(next), PACK(0, 0));
        return prev;
    } else {
        return p;
    }
}

/*
    place the point
*/
void *place(void* p, size_t size)
{
    size_t oldsize = GET_SIZE(HDRP(p));
    PUT(HDRP(p),  PACK((size + DSIZE), 1));
    PUT(FTRP(p), PACK((size + DSIZE), 1));
    if (size + DSIZE != old_size){
        void *next = NEXT_BLKP(p);
        size_t next_size = old_size - size - DSIZE;
        PUT(HDRP(p),  PACK(next_size, 1));
        PUT(FTRP(p), PACK(next_size, 1));       
    }
    return p;
}

/*
    sreach the whole list , and find the first empty block which size is enough
*/
void *find_fit(size_t size)
{
    void* p = heap_list;
    while (GET_SIZE(p) != 0) // if size == 0 , it point to end
    {
        if (GET_ALLOC(p) == 0 && (GET_SIZE(p) - DSIZE) >= size)
            return place(p, size);
        p = NEXT_BLKP(p);
    }
    return NULL;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    void *heap_list = mem_sbrk(4);
    PUT(heap_list, 0);
    PUT(heap_list + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_list + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_list + (3*WSIZE), PACK(0, 1));
    heap_list += (2*WSIZE);
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) return -1;
    return 0;
}

/*
    return true if valic else return false;
*/
bool mm_check()
{
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    char* bp;
    size_t newsize = (size + DSIZE + DSIZE-1) / WSIZE;
    bp = find_fit(newsize);
    // if the heap donit have enought memory, get more memory
    if (bp == NULL){
        size_t extend_heap_size = MAX(newsize, CHUNKSIZE);
        bp = extend_heap(extend_heap_size/WSIZE);
        if (bp == NULL) return NULL;
    }
    place(bp, newsize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














