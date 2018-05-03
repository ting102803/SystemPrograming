/*
 * mm-explicit.c - an empty malloc package
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 * @id : 201302423 
 * @name: 신종욱
 */
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

#define HDRSIZE		 4
#define FTRSIZE		 4
#define WSIZE		4
#define DSIZE		8
#define CHUNKSIZE	(1<<12)
#define OVERHEAD	8

#define MAX(x,y)	((x)>(y) ? (x) : (y))
#define MIN(x,y)	((x)<(y) ? (x) : (y))

#define PACK(size, alloc) ((unsigned)((size)|(alloc)))

#define GET(p)		(*(unsigned *)(p))
#define PUT(p, val)  (*(unsigned*)(p) = (unsigned)(val))


#define GET8(p)		(*(unsigned long *)(p))
#define PUT8(p,val) (*(unsigned long *)(p) = (unsigned long)(val))

#define GET_SIZE(p)		(GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

#define HDRP(bp)		((char *)(bp) - WSIZE)
#define FTRP(bp)		((char *)(bp) + GET_SIZE(HDRP(bp))	- DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE((char *)(bp)-DSIZE))

#define NEXT_FREEP(bp)	((char *)(bp))
#define PREV_FREEP(bp)	((char *)(bp)+WSIZE)

#define NEXT_FREE_BLKP(bp) 		((char *)GET8((char *)(bp)))
#define PREV_FREE_BLKP(bp)		((char *)GET8((char *)(bp) + WSIZE))
/*
 * Initialize: return -1 on error, 0 on success.
 */
void *h_ptr =0;
void *heap_start;
void *epilogue;
inline void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void *place(void *bp,size_t asize);
static void *coalesce(void *bp);
int mm_init(void) {
	if((h_ptr = mem_sbrk(DSIZE + 4*HDRSIZE))==NULL)
		return -1;
	heap_start = h_ptr;
	PUT(h_ptr ,NULL);//다음블럭을 가리키는 주소
	PUT(h_ptr+WSIZE,NULL);//이전블록을 가리키는 주소
	PUT(h_ptr+DSIZE,0);
	PUT(h_ptr +DSIZE+HDRSIZE,PACK(OVERHEAD,1));
	PUT(h_ptr +DSIZE+HDRSIZE+FTRSIZE, PACK(OVERHEAD,1));
	PUT(h_ptr +DSIZE+2*HDRSIZE+FTRSIZE,PACK(0,1));

	h_ptr += DSIZE+DSIZE;
	epilogue=h_ptr+HDRSIZE;

	if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
		return -1;

    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
   	char *bp;
	unsigned asize;
	unsigned extendsize;
	if(size<=0) return NULL;
	if(size<=DSIZE) {asize=2*DSIZE;}
	else asize=DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);


	if((bp =find_fit(asize)) !=NULL)
	{	place(bp,asize);
		return bp;}
	extendsize = MAX(asize,CHUNKSIZE);
	if((bp = extend_heap(extendsize/WSIZE))==NULL)
		return NULL;
	place(bp,asize);
	return bp;
}

/*
 * free
 */
void free (void *bp) {
    if(!bp) return;
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp),PACK(size,0));
	PUT(FTRP(bp),PACK(size,0));
	coalesce(bp);
}



/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
 
	size_t oldsize; 
	void *newptr; 
	if(size == 0)
	{ free(oldptr);  
	 return 0;}
	if(oldptr == NULL)
	{  return malloc(size);}
	newptr = malloc(size); 
	if(!newptr){  return 0;}
	oldsize = GET_SIZE(oldptr);
	if(size < oldsize)	
	{memcpy(newptr,oldptr, size);}
	else {memcpy(newptr,oldptr,oldsize);}

	free(oldptr); 
	 return newptr;}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
	size_t bytes = nmemb * size;
	void *newptr;

	newptr = malloc(bytes);
	memset(newptr, 0, bytes);

	return newptr;
}
inline  void *extend_heap(size_t words){
	unsigned *old_epilogue;
	char *bp;
	unsigned size;

	size = (words%2) ? (words+1)*WSIZE : words*WSIZE;
	if((long)(bp = mem_sbrk(size))<0)
	{return NULL;}
	
	old_epilogue = epilogue;
	epilogue = bp+size-HDRSIZE;

	PUT(HDRP(bp), PACK(size,0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(epilogue, PACK(0,1));
	return coalesce(bp);
}
static void *find_fit(size_t asize){
	char *bp;
	for(bp=heap_start;bp!=NULL;bp=((char *)GET(NEXT_FREEP(bp)))){

		if(asize <= GET_SIZE(HDRP(bp)))
		{return bp;	}//free리스트를  보면서알맞은 크기를 찾는다
}
	return NULL;
}

static void *place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	
	if((csize-asize)>=(2*DSIZE)){
		//블럭의 사이즈와입력받은 사이즈 차가 16이하라면
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp),PACK(asize,1));
		bp=NEXT_BLKP(bp);
		//블럭의 헤더와 풋터에 입력사이즈 만큼 할당됬다고한뒤 다음 블럭으로간다

		PUT(bp,GET(PREV_BLKP(bp)));
		PUT(bp+WSIZE,GET(PREV_BLKP(bp)+WSIZE));
//할당 받고 남은 블럭의 다음,이전 블록을 가리키는 주소에 원래의 다음,이전 블럭 주소로 바꾼다
		PUT(HDRP(bp),PACK(csize-asize,0));//할당받고 남은 사이즈를 저장
		PUT(FTRP(bp),PACK(csize-asize,0));//할당받고 남은 사이즈를 풋터에 저장
		PUT(GET(bp+WSIZE),bp);
		if(GET(bp)!=NULL)
		{PUT(GET(bp)+WSIZE,bp);}
	//남은블럭의 다음이 있다면 다음블럭의 이전을 bp로 저장한다
	
	}else{//차가 16보다 작으면 블럭을 나누지않고 모두할당한다
		PUT(HDRP(bp),PACK(csize,1));	
		PUT(FTRP(bp),PACK(csize,1));
		//해당블록의 헤더와 풋터에 할당여부를 저장한다
		PUT(GET(bp+WSIZE),GET(bp));
		
			if(GET(bp)!=NULL){ PUT(GET(bp)+WSIZE,GET(bp+WSIZE));}
			//블럭의 다음블럭의 이전을 가리키는 주소로 할당받은 블럭의 이전을 저장
	}
	
}

static void *coalesce(void *bp){
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	
	if(prev_alloc && next_alloc){//앞뒤 블럭이 모두 할당될경우
	if(GET(heap_start)==NULL)//만약아직 freelist가 비였다면
	{ PUT(heap_start,bp);//root를 새롭게 지정하고
	  PUT(bp,NULL);//bp의 next는 NULL로 지정
	  PUT(bp+WSIZE,heap_start);//bp의 이전블록은 root를 저장
	}else{
		PUT(bp,GET(heap_start));//bp의 next는 루트의 next를 받고
		PUT(bp+WSIZE,heap_start);//bp의 이전은 루트를 받는다
		if(GET(bp)!=NULL)
			{PUT(GET(bp)+WSIZE,bp);}//다음블록의 이전블록은 bp로 지정한다
		PUT(heap_start,bp);//루트의 next는 bp를 지정한다
	}}
	else if (prev_alloc && !next_alloc){//뒷블럭이 free일경우
		PUT(GET(NEXT_BLKP(bp)+WSIZE),GET(NEXT_BLKP(bp)));
		//프리된  블럭의 다음다음블럭의 이전은 다음블럭으로 지정한다 
		if(GET(NEXT_BLKP(bp))!=NULL) 
		{PUT(GET(NEXT_BLKP(bp))+WSIZE,GET(NEXT_BLKP(bp)+WSIZE));}
		//다음블럭이 존재한다면 다음다음블러의 이전블럭을 다음블록의 이전블록으로 한다
		PUT(bp,GET(heap_start));//bp의 다음블록은 원래 root가 가리키던걸 받는다
		PUT(bp+WSIZE,heap_start);//bp의 이전블럭은 root를 가리킨다
		if(GET(bp)!=NULL) {PUT(GET(bp)+WSIZE,bp);}
		//다음블럭이 존재하면 다음블럭의 이전주소에 프리된블럭을 지정
		PUT(heap_start,bp);//root의 다음블록에는 bp를 지정한다
		size+= GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));//free된 블럭의 헤더와 풋터에 두사이즈합을 저장
	}
	else if (!prev_alloc && next_alloc) {//앞이 free인 경우
		PUT(GET(PREV_BLKP(bp)+WSIZE),GET(PREV_BLKP(bp)));
		//이전이전 블럭의 다음블럭을 가리키는 주소에 이전의 다음블럭주소를 저장
		if(GET(PREV_BLKP(bp))!=NULL)
		{PUT(GET(PREV_BLKP(bp))+WSIZE,GET(PREV_BLKP(bp)+WSIZE));}
		//이전의 다음블럭이 존재한다면
		//이전블럭의 다음블럭의 이전을 가리키는 주소에 이전블럭의 이전을 저장
		PUT(PREV_BLKP(bp),GET(heap_start));
		PUT(PREV_BLKP(bp)+WSIZE,heap_start);
		//이전의 다음,이전블럭을 가리키는 주소에 root의 다음과 root를 저장

		if(GET(PREV_BLKP(bp))!=NULL)
		{PUT(GET(PREV_BLKP(bp))+WSIZE,PREV_BLKP(bp));}
		//이전의 다음블럭이 존재하면 프리된 이전블럭을 가리키는 주소로 변경
		PUT(heap_start,PREV_BLKP(bp));
	//root의 다음블럭을 가리키는 주소에 블럭의 이전블럭을 저장
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		//사이즈를 키우고 헤더와 풋터에 사이즈를 넣어준다
		bp = PREV_BLKP(bp);//앞으로 땡겨야함으로 옮겨준다
	}
	else {//그외 앞뒤로 free가있을경우이다
		PUT(GET(PREV_BLKP(bp)+WSIZE),GET(PREV_BLKP(bp)));
		//일단 이전이전블럭의 다음블럭 가리키는 주소에 이전의 다음블록으로 지정
		if(GET(PREV_BLKP(bp))!=NULL)
		{PUT(GET(PREV_BLKP(bp))+WSIZE,GET(PREV_BLKP(bp)+WSIZE));}
		//이전의 다음블럭이 존재한다면 적절히 포인터를 옮긴다
		PUT(GET(NEXT_BLKP(bp)+WSIZE),GET(NEXT_BLKP(bp)));
		// 다음블럭의 인의 다음블럭주소에 다음다음블럭을 저장한다
		if(GET(NEXT_BLKP(bp))!=NULL)
		{PUT(GET(NEXT_BLKP(bp))+WSIZE,GET(NEXT_BLKP(bp)+WSIZE));}
	//만약 다음다음블럭이 존재한다면 적절히 포인터를 올긴다
		
		//이제 이전블럭을 고쳐주자
		PUT(PREV_BLKP(bp),GET(heap_start));
		// 이전의 다음블럭에 root의 다음블럭을 저장
		PUT(PREV_BLKP(bp)+WSIZE,heap_start);
		//이전이전 블럭을 가리키는 주소에 root를 저장
		if(GET(PREV_BLKP(bp))!=NULL)
		{PUT(GET(PREV_BLKP(bp))+WSIZE,PREV_BLKP(bp));}
		//블럭의 이전블럭이 존재하면 이전의 다음의 이전을 가리키는 주소를 바꾼다
		PUT(heap_start,PREV_BLKP(bp));
		//root의 다음블럭을 free된 블럭의 이전을 저장(어차피 합칠꺼니깐)
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		//사이즈를 더해준뒤에 다시 블럭마다 사이즈를 넣어주면서 새로운 bp를 만든다
	}
	
	return bp;
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

/*
 * mm_checkheap
 */
void mm_checkheap(int verbose) {
}


