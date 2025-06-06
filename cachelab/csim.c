#include "cachelab.h"
// 定义一行的结构
struct line {
    int valid;
    int tag;
    int last_used;
};

// 定义组，包含E个行
typedef struct line* set;
// 定义缓存，包含S个组
set* cache;
// 定义缓存全局变量
int s, E, b, t, timestamp;
// 定义需要返回的全局变量
int hit = 0, miss = 0, eviction = 0;

// 模拟缓存
void caching(int address, int is_modified) {
    // 对地址进行解码
    // 组索引
    int set_index = (address >> b) & ((1 << s) - 1);
    // tag标记
    int tag = address >> (b + s);

    // 根据组索引获得所在的组
    set cur_set = cache[set_index];

    // 循环匹配相应的行
    for (int i = 0; i < E; i++) {
        // 当前行
        struct line cur_line = cur_set[i];
        if (cur_line.tag == tag) {
            // 说明匹配上了
            if (cur_line.valid != 1) {
                // 说明是冷不命中
                eviction++;
                miss++;
                cur_line.valid = 1;
            }
            // hit+1
            hit++;
        }
    }
    // 循环了所有的行发现没有匹配上
    // 未命中
    miss++;
}
int main()
{
    printSummary(0, 0, 0);
    return 0;
}
