/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    // 由于从第二次读A开始，已经读到的B的相应行会被驱逐，所以会多很多miss
    // 优化原理是先将数据全部加载进B然后在B内进行转置，这样就不用担心缓存问题了，因为B的全部数据都已经被加载
    if (M == 32) {
        int a, b, c, d, e, f, g, h;
        for (int i = 0; i < N; i += 8) {
            for (int j = 0; j < M; j+= 8) {
                for (int k = 0; k < 8; k++) {
                    // 按行读A
                    a = A[i + k][j + 0];
                    b = A[i + k][j + 1];
                    c = A[i + k][j + 2];
                    d = A[i + k][j + 3];
                    e = A[i + k][j + 4];
                    f = A[i + k][j + 5];
                    g = A[i + k][j + 6];
                    h = A[i + k][j + 7];

                    // 按行写B
                    B[j + k][i + 0] = a;
                    B[j + k][i + 1] = b;
                    B[j + k][i + 2] = c;
                    B[j + k][i + 3] = d;
                    B[j + k][i + 4] = e;
                    B[j + k][i + 5] = f;
                    B[j + k][i + 6] = g;
                    B[j + k][i + 7] = h;
                }
                // 对B转置
                for (int k = 0; k < 8; k++) {
                    for (int l = 0; l < k; l++) {
                        a = B[j + k][i + l];
                        B[j + k][i + l] = B[j + l][i + k];
                        B[j + l][i + k] = a;
                    }
                }
            }
        }
    }   


    if (M == 64) {
        // 整体的思路是：
        // 按照8*8来读A
        // 然后将整个大块分成4*4的小块，每块分别用[0][0],[0][1],[1][0],[1][1]来标记
        // 通过借用右边已经warm-up的部分来减少miss
        int a, b, c, d, e, f, g, h;
        for (int i = 0; i < N; i += 8) {
            for (int j = 0; j < M; j += 8) {
                // 先处理A 8*8的上半部分
                // 也就是读入四行
                for (int k = 0; k < 4; k++) {
                    a = A[i + k][j + 0];
                    b = A[i + k][j + 1];
                    c = A[i + k][j + 2];
                    d = A[i + k][j + 3];
                    e = A[i + k][j + 4];
                    f = A[i + k][j + 5];
                    g = A[i + k][j + 6];
                    h = A[i + k][j + 7];

                    // 然后我们先将a[0][0]转置到b[0][0]
                    B[j + 0][i + k] = a;
                    B[j + 1][i + k] = b;
                    B[j + 2][i + k] = c;
                    B[j + 3][i + k] = d;
                    
                    // 成功转置后我们借助已经warm-up的行的后半部分
                    // 也就是b[0][1]
                    // 暂时存放我们转置好的a[0][1]
                    B[j + 0][i + k + 4] = e;
                    B[j + 1][i + k + 4] = f;
                    B[j + 2][i + k + 4] = g;
                    B[j + 3][i + k + 4] = h;
                }
                // 以上，我们的A 8*8的上半部分已经成功读入完毕
                // 下面处理A 8*8的下半部分
                // 下半部分处理的思路就是
                // 将暂存到b[0][1]的部分移动到b[1][0]
                // 然后将a[1][0]的转置写入b[0][1]
                for (int k = 0; k < 4; k ++) {
                    
                    e = A[i + 4][j + k];
                    f = A[i + 5][j + k];
                    g = A[i + 6][j + k];
                    h = A[i + 7][j + k];

                    // 注意这里b[0][1]要按行读取
                    // 如果按列读取
                    // 那你在写到b[0][1]的时候会有很多冲突不命中
                    a = B[j + k][i + 4];
                    b = B[j + k][i + 5];
                    c = B[j + k][i + 6];
                    d = B[j + k][i + 7];
                    
                    // 注意：这里改变赋值顺序也会增大miss
                    // 也就是说你读完之后就把相应的位置写上时最好的
                    B[j + k][i + 4] = e;
                    B[j + k][i + 5] = f;
                    B[j + k][i + 6] = g;
                    B[j + k][i + 7] = h;

                    // 复制到b[1][0]
                    B[j + k + 4][i + 0] = a;
                    B[j + k + 4][i + 1] = b;
                    B[j + k + 4][i + 2] = c;
                    B[j + k + 4][i + 3] = d;
                }

                // 最后转置a[1][1]到b[1][1]即可
                // 这里直接借鉴32 * 32时的思路
                // 先复制再转置
                for (int k = 0; k < 4; k++) {
                    a = A[i + k + 4][j + 4];
                    b = A[i + k + 4][j + 5];
                    c = A[i + k + 4][j + 6];
                    d = A[i + k + 4][j + 7];
                    B[j + k + 4][i + 4] = a;
                    B[j + k + 4][i + 5] = b;
                    B[j + k + 4][i + 6] = c;
                    B[j + k + 4][i + 7] = d;
                }

                for (int k = 4; k < 8; k++) {
                    for (int l = 4; l < k; l++) {
                        a = B[j + k][i + l];
                        B[j + k][i + l] = B[j + l][i + k];
                        B[j + l][i + k] = a;
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

