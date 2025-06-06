#include <getopt.h>
#include <stdio.h>
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
int v = 0, s, E, b, t, timestamp;
// 定义需要返回的全局变量
int hit = 0, miss = 0, eviction = 0;

// 模拟缓存
// is_modified 是指这条代码是否是修改（写）指令
// 如果是写指令的话，它包括load+store两个操作
// 相应的hit应该在自增的基础上再加一
// 因为load已经把数据成功加载到缓存，store时当然可以命中
void caching(size_t address, int is_modify) {
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
            hit += is_modify;
            // 更新当前行的LRU
            cur_line.last_used_time = timestamp;
            // 支持v选项
            if (v) {
                printf("hit\n");
            }
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
    hit += is_modify;

    // 数据还没有被加载到缓存中
    // 因此对应的 tag（标记位）在缓存中也还没有出现过
    // 所以当未命中时才有机会发生冷不命中
    // 判断是否是冷不命中，只需要根据LRU_time是不是我们设置的初始值即可
    // 这是因为LRU_time是我们遍历所有行得到的结果
    // 如果它是初始值说明一定会发生冷不命中
    eviction += (LRU_time != -1);

    if (v) {
        if (LRU_time != -1) {
            if (is_modify)
                printf("miss eviction hit\n");
            else
                printf("miss eviction\n");
        }
        else {
            printf("miss\n");
        }
    }

    // 没有命中的话就要驱逐了
    // 我们之前记录的LRU位置正好派上用场
    // 先要进行时间更新，因为我们已经使用LRU这一行了
    cur_set[LRU_pos].last_used_time = timestamp;
    // 然后替换数据，也就是置换相应的tag
    cur_set[LRU_pos].tag = tag;
}

// 支持h参数
void print_usage() {
    puts("Usage: ./cism-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>");
    puts("Options:");
    puts("-h:                print usage info                        ");
    puts("-v:                displace trace info                     ");
    puts("-s <s>:            Numbers of set index bits               ");
    puts("-E <E>:            Associativity (numbers of lines per set)");
    puts("-b <b>:            Numbers of block bits                   ");
    puts("-t <tracefile>:    Name of the valgrind trace to replay    ");
    puts("");
    puts("Examples:");
    puts("linux> ./cism-ref -s 4 -E 1 -b 4 -t traces/yi.trace");
}

int main(int argc, char* argv[])
{
    // 读取命令行参数
    int option;
    FILE* trace_file;
    // 获取参数
    if (argc == 1) {
        print_usage();
        exit(0);
    }
    // 这里getopt()的三个参数表示标签，后面带：的字母表示必须要加上选项参数
    while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (option) {
            case 'h':
                print_usage();
                exit(0);
            case 'v':
                v = 1;
                break;
            case 's':
                // 这些是#include <getopt.h>头文件里定义的：外部变量 optarg 指向当前选项参数的指针，atoi将字符串转换为整数
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                trace_file = fopen(optarg, "r");
                break;
        }
    }

    // 校验参数
    if (s <= 0 || E <= 0 || b <= 0 || s + b > 64 || trace_file == NULL) {
        print_usage();
        exit(1);
    }

    // 初始化缓存
    // s 是 set index的bit数目
    // cache中有2^s^个组
    cache = (set *)malloc(sizeof(set) * (1 << s));
    // 初始化组
    for (int i = 0; i < (1 << s); i++) {
        set cur_set = cache[i];
        cur_set = (set)malloc(sizeof(struct line) * E);
        // 初始化行
        for (int j = 0; j < E; j++) {
            cur_set[j].valid = -1;
            cur_set[j].tag = -1;
            cur_set->last_used_time = -1;
        }
    }

    // 下面读文件，根据文件更新hit，miss和eviction
    // 文件每一行的格式是
    // operation address,size

    // 储存读到的指令
    char operation;
    // 储存得到的地址
    size_t address;
    // 储存读到的size
    int size;

    while (fscanf(trace_file, "%s %ld,%d\n", &operation, &address, &size) == 3) {
        // 每读一行就更新时间戳
        timestamp++;
        if (v) {
            printf("%c %lx,%d ", operation, address, size);
        }
        // 根据读到的指令操作来选择cache的行为
        switch (operation) {
            case 'I':
                // 我们不对指令缓存做处理
                continue;
            case 'M': // Load + Store
                // 修改操作
                caching(address, 1);
                break;
            case 'L': // Load
                case 'S': // Store
                caching(address, 0);
        }
    }
    // 程序结束，释放缓存
    free(cache);
    printSummary(hit, miss, eviction);
    return 0;
}
