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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 自定义宏
// 单字大小
// 双字大小
#define WSIZE 4
#define DSIZE 5
// 按照2^12=2KB（字节）拓展堆
#define CHUNKSIZE (1 << 12)

// 最大值和最小值
#define MAX(x, y)        ((x) > (y) ? (x) : (y))
#define MIN(x, y)        ((x) < (y) ? (x) : (y))

// 利用有效负载为8的倍数，最低位存放标识位(ALLOC)
#define PACK(size, alloc)    ((size) | (alloc))
#define PACK_ALL(size, prev_alloc, alloc)   ((size) | (prev_alloc) | (alloc))

// 读写一个字（4B），用于设置和获取头部和尾部
#define GET(p)      (*(unsigned*)(p))
#define PUT(p, val) (*(unsigned*)(p) = val)

// 获取块的大小和分配位标志
// 最低位为当前块的分配标识位，次低位为前一个块分配标识位
// size为块大小，即包括头部和尾部的大小
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV(p) (GET(p) & 0x2)
#define SET_ALLOC(p) (GET(p) |= 0x1)
#define SET_FREE(p) (GET(p) &= ~0x1)
#define SET_PREV_ALLOC(p) (GET(p) |= 0x2)
#define SET_PREV_FREE(p) (GET(p) &= ~0x2)

// 获得块头部和尾部
// 在整个编程过程中，一定要注意，除了特殊的序言块 / 结尾块，bp 指向的永远是：
// 分配块：有效负载的第一个字节
// 空闲块：prev 指针偏移的第一个字节
#define HDRP(bp)    ((char *)(bp) - WSIZE) // 这里是利用指针运算
#define FTRP(bp)    (((char *)(bp) + GET_SIZE(HDRP(bp))) - DSIZE) // 得到的是bp所指向位置的块的块尾部

// 获得前一个块和后一个块
// 获得前一个块支队前一个块为空闲块有效，因为分配块没有脚部
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)) // 这里参考空闲块的结构图，这里实际上计算得到的是指向下一个空闲块的指针
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)))

/* 全局变量 */
// 指向堆的起始位置
static char* heap_listp = 0;

/* 空闲链表配置 */
#define FREE_LIST_NUM 15
// 空闲链表的头指针数组，每个元素都是一个头指针，指向该类空闲列表的首个空闲块
static char** free_lists;
/* 空闲链表便利操作 */
// 参考空闲链表结构
#define PREV_NODE(bp)   ((char *)(mem_heap_lo() + *(unsigned*)(bp)))
#define NEXT_NODE(bp)   ((char *)(mem_heap_lo() + *(unsigned*)(bp + WSIZE)))
#define SET_NODE_PREV(bp, val)  (*(unsigned*)(bp) = ((unsigned)(long)val))
#define SET_NODE_NEXT(bp, val)  (*(unsigned*)((char *)bp + WSIZE) = ((unsigned)(long)val))

/* 检查函数用 */
// 检查指针是否对其 8 字节
#define CHECK_ALIGN(p)  (ALIGN(p) == (size_t)p)
// 检查空闲链表节点是否符合当前链表（桶）的设置范围
static inline void get_range(size_t index);
static size_t low_range;
static size_t high_range;

/* 辅助函数原型 */
static inline void* extend_heap(size_t words);
static inline void* coalesce(void* bp, size_t size);// 合并
static inline size_t get_index(size_t size);
static inline size_t adjust_alloc_size(size_t size);
static inline void* find_fit(size_t asize);
static inline void place(void* bp, size_t size);
static inline void insert_node(void* bp, size_t size);
static inline void delete_node(void* bp);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 初始化空闲链表 - 注意空闲链表放在堆负载位置之前
    free_lists = mem_heap_lo();
    // 为每一桶分配空间，大小为DSIZE
    int i = 0;
    while (i < FREE_LIST_NUM) {
        // mem_sbrk()失败时会返回(void*) - 1
        if ((heap_listp = mem_sbrk(DSIZE)) == (void*) - 1) {
            return -1;
        }
        heap_listp[i] = mem_heap_lo();
        i++;
    }
    // 上面的分配进行完后，已经是对齐的
    // 我们仍需要开辟两个DSIZE的新空间来存储序言块和尾块
    if ((heap_listp = mem_sbrk(2 * DSIZE)) == (void*) - 1) {
        return -1;
    }

    // 没有返回说明开辟成功
    // 下面分配相应的部分
    // 空字块对齐序言块头
    PUT(heap_listp, 0);
    // 序言块头
    PUT(heap_listp + WSIZE, PACK(DSIZE, 1));
    // 序言块尾
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    // 结尾块头
    PUT(heap_listp + (3 * WSIZE), PACK(0, 3));
    // 更新heap_listp
    // 只加1个DSIZE使得指针在序言块头和尾之间
    // 这一点跟课本上相呼应
    heap_listp += DSIZE;

    // 拓展堆
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
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














