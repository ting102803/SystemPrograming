/*
 * mm-implicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : c201302423 
 * @name :   신종욱        */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define OVERHEAD 8

#define MAX(x,y) ((x)>(y)?(x):(y))
#define PACK(size,alloc) ((size)|(alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))

#define GET_SIZE(p) (GET(p)&~0x7)
#define GET_ALLOC(p) (GET(p)&0x1)
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))


int mm_init(void);
static void *find_fit(size_t asize);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);

static char *heap_listp = 0;
void *good_fit;

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	//초기 empty heap을 생성
	if ((heap_listp = mem_sbrk(4*WSIZE)) == NULL)
		return -1;
	
	PUT(heap_listp, 0); //정령을 위한 무의미한값
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // Prologue header 
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // Prologue footer 
	PUT(heap_listp + (3*WSIZE), PACK(0, 1)); // Epilogue header 
	heap_listp += (2*WSIZE);
	good_fit=heap_listp;
	//empty heap을 free block으로 확장
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

static void *extend_heap(size_t words){//요청받은 size의 빈블록을 만든다
	char *bp;
	size_t size;

	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
	
	
	PUT(HDRP(bp),PACK(size,0)); /* Free block header */
	PUT(FTRP(bp),PACK(size,0)); /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

	return coalesce(bp);
}

/*
 * malloc
 */
void *malloc (size_t size) {//블록을 할당하는 함수

	size_t asize; 
	size_t extendsize; 
	char *bp;
		
	if (size == 0)	return NULL;
	
	if (size <= DSIZE) asize = 2*DSIZE;
	else asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
	
	if ((bp =find_fit(asize)) != NULL) {
		place(bp, asize);//해당 사이즈만큼 포인터를 이동
		return bp;}
	
	extendsize = MAX(asize,CHUNKSIZE);
	if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
	
	place(bp, asize);
	good_fit = NEXT_BLKP(bp);//현재의 다음힙의 위치를 저장한다
	return bp;
}

static void *find_fit(size_t asize){

	char *bp;
	for(bp=good_fit;GET_SIZE(HDRP(bp))>0;bp=NEXT_BLKP(bp))
		if(!GET_ALLOC(HDRP(bp))&&(asize <= GET_SIZE(HDRP(bp))))
			return bp;
	//가장최근에 생성된 block 부터 끝가지 서치하여 기록할수있는
	//freeblock을 찾아내어 반환한다.
	
	return NULL;
}



static void place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));//사이즈를 구함
	
	if((csize-asize)>=(2*DSIZE)){//bp의 사이즈가 더클경우
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp),PACK(asize,1));
		bp=NEXT_BLKP(bp);
		PUT(HDRP(bp),PACK(csize-asize,0));
		PUT(FTRP(bp),PACK(csize-asize,0));
	}else{//작을경우
		PUT(HDRP(bp),PACK(csize,1));	
		PUT(FTRP(bp),PACK(csize,1));	}}
		//헤당하는 사이즈만큼 옮겨준다

void free (void *bp) {
	if(bp == 0) return;
	
	size_t size = GET_SIZE(HDRP(bp));//헤더에서 size를 읽음
	
	PUT(HDRP(bp), size);
	PUT(FTRP(bp), size);//bp의 header와 footer에 size 변경
	coalesce(bp);//주위를 살피고 병합한다

}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
	size_t oldsize;
  	void *newptr;
    if(size == 0) {
		free(oldptr);
		return 0;
	}
	if(oldptr == NULL) {
		return malloc(size);
	}

	newptr = malloc(size);
	if(!newptr) {
		return 0;
	}
	oldsize = GET_SIZE(oldptr);
	if(size < oldsize) oldsize = size;
		memcpy(newptr, oldptr, oldsize);
	free(oldptr);

	return newptr;
				
}


void *calloc (size_t nmemb, size_t size) {
	size_t bytes = nmemb * size;
	void *newptr;

	newptr = malloc(bytes);
	memset(newptr, 0, bytes);

	return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p < mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

static void *coalesce(void *bp){// 빈공간을 살펴 합치는 함수
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	
	if(prev_alloc && next_alloc){
		return bp;
	}//앞뒤 블럭이 둘다 할당되어있을경우에는 바로 리턴
	else if (prev_alloc && !next_alloc){
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
	}//다음블럭이 freeblock일경우 같이 병합
	else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}//이전 블럭이 freeblock일경우 같이 병합
	else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
		GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}//양쪽다 비할당일경우 둘다 합치고 리턴한다


	if (good_fit >bp)  good_fit = bp;//현재를 가리키는 포인터값이 더클경우
	//이전 블록도 같이 병합됬다는것이니 가르키는 포인터를 옮긴다
	return bp;
}


/*
* mm_checkheap
 */
void mm_checkheap(int verbose) {
}
