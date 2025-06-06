#include <stdlib.h>
#include <time.h>

#include "cachelab.h"
// 定义一行的结构
struct line {
    int valid;
    int tag;
    int last_used_time;
};

// 定义组，包含E个行
typedef struct line* set;
// 定义缓存，包含S个组
set* cache;
// 定义缓存全局变量，timestamp是我们维护的时间戳，用来执行LRU替换
int s, E, b, t, timestamp;
// 定义需要返回的全局变量
int hit = 0, miss = 0, eviction = 0;

// 模拟缓存
// is_modified 是指这条代码是否是修改（写）指令
// 如果是写指令的话，它包括load+store两个操作
// 相应的hit应该在自增的基础上再加一
// 因为load已经把数据成功加载到缓存，store时当然可以命中
void caching(size_t address, int is_modified) {
    // 对地址进行解码
    // 组索引
    int set_index = (address >> b) & ((1 << s) - 1);
    // tag标记
    int tag = address >> (b + s);

    // 根据组索引获得所在的组
    set cur_set = cache[set_index];
    // 设置初始的LRU位置和时间
    int LRU_pos = 0, LRU_time = cur_set[0].last_used_time;

    // 循环匹配相应的行
    for (int i = 0; i < E; i++) {
        // 当前行
        struct line cur_line = cur_set[i];
        if (cur_line.tag == tag) {
            hit++;
            hit += is_modified;
            // 更新当前行的LRU
            cur_line.last_used_time = timestamp;
            // 已经命中，无需再继续执行
            return;
        }
        // 更新LRU位置和时间
        if (cur_line.last_used_time < LRU_time) {
            LRU_pos = i;
            LRU_time = cur_line.last_used_time;
        }
    }
    // 循环下来没有命中，miss++
    miss++;
    // 如果是写操作的话，尽管第一次没有命中，进行驱逐后store的时候还是会命中
    // 所以hit+is_modify
    hit += is_modified;

    // 数据还没有被加载到缓存中
    // 因此对应的 tag（标记位）在缓存中也还没有出现过
    // 所以当未命中时才有机会发生冷不命中
    // 判断是否是冷不命中，只需要根据LRU_time是不是我们设置的初始值即可
    // 这是因为LRU_time是我们遍历所有行得到的结果
    // 如果它是初始值说明一定会发生冷不命中
    eviction += (LRU_time != -1);

    // 没有命中的话就要驱逐了
    // 我们之前记录的LRU位置正好派上用场
    // 先要进行时间更新，因为我们已经使用LRU这一行了
    cur_set[LRU_pos].last_used_time = timestamp;
    // 然后替换数据，也就是置换相应的tag
    cur_set[LRU_pos].tag = tag;
}
int main()
{
    printSummary(0, 0, 0);
    return 0;
}
