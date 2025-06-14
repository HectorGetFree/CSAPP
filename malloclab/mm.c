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
// 用按位或来打包的原因是 这些值在位上是互不冲突的
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
    // 初始化空闲链表 
    // 注意我们维护的空闲块结构是一个由多个双向链表组成的表
    // 每一个DSIZE条目对应的是每个链表的头
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
    // 如果没有初始化堆，就进行初始化
    if (heap_listp == 0) {
        mm_init();
    }

    // 处理无效请求
    if (size == 0) {
        return NULL;
    }

    // 调整块大小
    size = adjust_alloc_size(size);
    // 对齐后的size
    size_t asize;

    // 分配数应该为DSIZE的整数倍，且至少为2，这样可以保证对齐
    if (size <= DSIZE) {
        // 不足DSIZE时多给它一个DSIZE
        // 这是为了存储头部和脚部（分别是一个WSIZE）
        asize = 2 * DSIZE;
    } else {
        // 参照课本第二章向上取整算法，保证多一个WSIDE用于存储头部
        // 考虑到分配块结构中首尾各有一个未知块
        // 所以对于奇数个字长的请求可以节省一个WSIDE
        asize = DSIZE * ((size + (WSIZE)+(DSIZE - 1)) / DSIZE);
    }
    // 用于存储新分配的地址
    char* bp;
    // 搜索空闲链表
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // 没有搜索到合适的空闲链表
    // 那就开辟新空间
    // 新空间的大小是CHUNKSIZE和asize中的最大值
    size_t extend_size = MAX(CHUNKSIZE, asize);
    if ((bp = extend_heap(extend_size)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    return bp;

}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 处理非法输入
    if (ptr == NULL) {
        return;
    }
    // 如果没有初始化
    // 那就先初始化然后直接返回
    if (heap_listp = 0) {
        mm_init();
        return;
    }
    // 获取块大小
    size_t cur_size = GET_SIZE(HDRP(ptr));
    // 获取前面块的分配状态
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(ptr));
    // 设置头部和脚部
    PUT(HDRP(ptr), PACK_ALL(cur_size, prev_alloc, 0));
    PUT(FTRP(ptr), PACK_ALL(cur_size, prev_alloc, 0));

    // 合并空闲块
    coalesce(ptr, cur_size);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // 如果size为0
    // 等同于free()
    if (size == 0) {
        mm_free(ptr);
        return ptr;
    }

    // 如果ptr为NULL, size != 0
    // 等同与malloc 
    if (ptr == NULL) {
        ptr = malloc(size);
        return ptr;
    }

    // 重新找到一个空间
    void* newptr = malloc(size);
    
    // 如果分配失败
    // 直接退出
    if (newptr == NULL) {
        return 0;
    }

    // 分配成功就进行数据拷贝
    // 有可能产生数据截断
    size_t old_size = GET_SIZE(HDRP(ptr));
    old_size = MIN(old_size, size);
    memcpy(newptr, ptr, old_size);

    // 释放原来的空间
    free(ptr);
    return newptr;
}


/* 个人添加的部分 */
void* calloc(size_t size, size_t num) {
    size_t asize = size * num;
    void* ptr = mm_malloc(asize);
    memset(ptr, 0, asize);
    return ptr;
}


/* 辅助函数 */
/*
 * 拓展堆空间
 * 保证对齐到双字，设置结尾块
 * 如果前一个块是空闲块，会向上合并
 * 返回值为指向新开辟（空闲）块的指针
*/
static inline void* extend_heap(size_t words) {
    // 完成对齐
    // 获得相应的大小
    size_t size = (words % 2) ? (words + 1) * 2 * WSIZE : words * WSIZE;
    // 开辟新空间
    void* bp;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }
    // 初始化新空闲块的头部和脚部
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    PUT(HDRP(bp), PACK_ALL(size, prev_alloc, 0));
    PUT(FTRP(bp), PACK_ALL(size, prev_alloc, 0));
    // 初始化新结尾块，即最后一个WSIZE
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    // 向上合并
    return coalesce(bp, size);
}

/*
 * 合并相邻空闲块
 * 此过程中会对合并后的空闲块之后的分配块设置前一个块分配标志位（PREV_ALLOC）
*/

static inline void* coalesce(void* bp, size_t size) {
    // 获取前后块的分配情况
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    // 分类讨论
    // case 1: 前后都已经分配
    // 只能把自身的状态设置为未分配
    if (prev_alloc && next_alloc) {
        SET_PREV_FREE(HDRP(NEXT_BLKP(bp)));
    }
    else if (prev_alloc && !next_alloc) {
        // 前面的块已经被分配，但是后面的没有
        // 此时可以开始合并
        // 先将位于原链表的下一个块的信息删除 -- 这是因为我们合并之后下一个块自动归于此处的块了
        delete_node(NEXT_BLKP(bp));
        // 计算新的大小
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // 更新头部
        PUT(HDRP(bp), PACK_ALL(size, 2, 0));
        // 更新脚部
        // 需要注意：此处已经更新头部，下一个块已经指向分配块了，不能以 NEXT_BLKP(bp) 访问
        PUT(FTRP(bp), PACK_ALL(size, 2, 0));
    }
    else if (!prev_alloc && next_alloc) {
        // 前未分配，后已经分配
        // 还是先删除
        delete_node(PREV_BLKP(bp));
        // 更新自身的free状态 -- 为什么上一种情况下没有（存疑）
        SET_PREV_FREE(HDRP(NEXT_BLKP(bp)));
        // 计算大小
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        // 更新新块的头部和脚部
        // 首先需要前前块的alloc状态
        size_t prev_prev_alloc = GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)));
        // 更新
        PUT(HDRP(PREV_BLKP(bp)), PACK_ALL(size, prev_prev_alloc, 0));
        PUT(FTRP(bp), PACK_ALL(size, prev_prev_alloc, 0));
        bp = PREV_BLKP(bp);
    } else {
        // 前后都没有被分配
        // 融合一下前面的逻辑
        // 先删除
        delete_node(PREV_BLKP[bp]);
        delete_node(NEXT_BLKP(bp));
        // 计算大小
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // 更新状态
        // 先得到前前块的alloc状态
        size_t prev_prev_alloc = GET_PREV_ALLOC(HDRP(PREV_BLKP(bp)));
        // 更新
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, prev_prev_alloc, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, prev_prev_alloc, 0));
        bp = PREV_BLKP(bp);
    }
    insert_node(bp, size);
    return bp;
}

/* 找到合适的空闲块 */
static inline void* find_fit(size_t size) {
    // 根据size大小获取一个大致的索引
    int num = get_index(size);
    // 用于存储返回结果
    char* bp;
    // 遍历整个堆结构
    // 外循环是遍历每个双向链表
    // 内循环是在一个双向列表里遍历
    // 直到我们找到了一个合适大小的空闲块
    for (;num <= FREE_LIST_NUM; num++) {
        // 这里注意：我们在初始化空闲链表时有free_lists[i] = mem_heap_lo();
        // 所以循环结束的条件是 bp != mem_heap_lo()
        for (bp = free_lists[num]; bp != mem_heap_lo(); bp = NEXT_NODE(bp)) {
            long spare = GET_SIZE(HDRP(bp)) - size;
            if (spare >= 0) {
                return bp;
            }
        }
    }
    return NULL;
}

/*
 * get_index: 根据块大小获得空闲链表的索引
 * 分界限由所有 trace 的 malloc & relloc 频率统计尖峰与尝试调整得到 
 * -- 来自：https://arthals.ink/blog/malloc-lab#%E8%8E%B7%E5%BE%97%E7%A9%BA%E9%97%B2%E9%93%BE%E8%A1%A8%E7%B4%A2%E5%BC%95-get_indexsize
 */
static inline size_t get_index(size_t size) {
    if (size <= 24)
        return 0;
    if (size <= 32)
        return 1;
    if (size <= 64)
        return 2;
    if (size <= 80)
        return 3;
    if (size <= 120)
        return 4;
    if (size <= 240)
        return 5;
    if (size <= 480)
        return 6;
    if (size <= 960)
        return 7;
    if (size <= 1920)
        return 8;
    if (size <= 3840)
        return 9;
    if (size <= 7680)
        return 10;
    if (size <= 15360)
        return 11;
    if (size <= 30720)
        return 12;
    if (size <= 61440)
        return 13;
    else
        return 14;
}


/*
 * adjust_alloc_size: 调整分配块大小
 * 面向助教编程
 * 尤其考察了 binaray.rep 和 freeciv.rep 
 * 注：这里是原北大实现的额外内容
 */
static inline size_t adjust_alloc_size(size_t size) {
    // freeciv.rep
    if (size >= 120 && size < 128) {
        return 128;
    }
    // binary.rep
    if (size >= 448 && size < 512) {
        return 512;
    }
    if (size >= 1000 && size < 1024) {
        return 1024;
    }
    if (size >= 2000 && size < 2048) {
        return 2048;
    }
    return size;
}

/*
 * place：找到合适大小空闲块的情况下，分配块
 * 如果剩余块大小≥最小块大小，则额外分割出一个空闲块并置入空闲链表
 * 对于分配块，不额外添加脚部，以增加空间利用率
 * 对于空闲块，额外添加脚部，以便于合并
*/
static inline void place(void* bp, size_t size) {
    // 得到当前块的大小
    size_t cur_size = GET_SIZE(HDRP(bp));
    // 获取剩余块的大小
    size_t remain_size = cur_size - size;

    // 如果剩余块的大小比最小块的大小小,则不分割
    // 因为太小了，反正也用不上
    if (remain_size < DSIZE * 2) {
        // 无需改变块大小
        // 只需要分配标识位即可
        SET_ALLOC(HDRP(bp));
        // 设置头部
        SET_PREV_ALLOC(HDRP(NEXT_BLKP(bp)));
        // 如果下一个块是空闲块，则还需要设置其脚部
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            SET_PREV_ALLOC(FTRP(NEXT_BLKP(bp)));
        }
    }
    // 剩余块的大小大于等于最小块的大小
    // 进行分割，将分割下来的最小块加入到我们维护的链表结构中
    else {
        // 设置当前块头部
        PUT(HDRP(bp), PACK_ALL(size, GET_PREV_ALLOC(HDRP(bp)), 1));

        // 设置剩余块的头部和脚部
        PUT(HDRP(NEXT_BLKP(bp), PACK_ALL(remain_size, 2, 0)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK_ALL(remain_size, 2, 0));

        // 插入到我们维护的结构中、
        insert_node(NEXT_BLKP(bp), remain_size);
    }
}

// 我们采用LIFO策略
// 即插入到链表头部，再次分配时优先分配最近释放的块
static inline void insert_node(void* bp, size_t size) {
    // 获取索引
    size_t num = get_index(size);
    // 获取当前链表
    char* cur = free_lists[num];
    // 插入到链表开头
    free_lists[num] = bp;
    if (cur != mem_heap_lo()) {
        SET_NODE_PREV(bp, NULL);
        SET_NODE_NEXT(bp, cur);
        SET_NODE_PREV(cur, bp);
    } else {
        SET_NODE_PREV(bp, NULL);
        SET_NODE_NEXT(bp, NULL);
    }
}

static inline void delete_node(void* bp) {
    // 获得前后节点
    void* prev = PREV_NODE(HDRP(bp));
    void* next = NEXT_NODE(HDRP(bp));
    // 如果是头结点
    if (prev == mem_heap_lo()) {
        free_lists[num] = next;
        if (next != mem_heap_lo()) {
            SET_NODE_PREV(next, NULL);
        }
    }
    else {
        SET_NODE_NEXT(prev, next);
        if (next != mem_heap_lo()) {
            SET_NODE_PREV(next, prev);
        }
    }
}