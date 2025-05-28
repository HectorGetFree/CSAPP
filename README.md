# CSAPP

## 未独立完成的lab部分思路记录

### data-lab

注意⚠️：本lab中的C代码为`C90/C89`标准,所有变量声明必须放在函数或代码块的开头，且不能在代码中出现`0b`标识的二进制常数，否则尽管可以通过测试，但是`./dlc bits.c`规范会提示`parse error`或者变量名`undeclared`,此外也不要出现没有使用过的变量

#### `int howManyBits(int x)`

```C
int howManyBits(int x) {
	int sign = x >> 31;
	// 如果是负数就取反，正数就不变
	// 最后将所得到的位数加上符号位这一位即可
	x = x ^ sign;

	int b16 = (!!(x >> 16)) << 4; // 如果高16位有1，那么就说明至少需要16位，没1结果就是0
	x = x >> b16; // 如果至少需要16位，那我们只需要再判断高16位；如果不需要，那我们依旧判断低16位

	// 下面同理
	int b8 = (!!(x >> 8)) << 3;
	x = x >> b8;

	int b4 = (!!(x >> 4)) << 2;
	x = x >> b4;

	int b2 = (!!(x >> 2)) << 1;
	x = x >> b2;

	int b1 = (!!(x >> 1));
	x = x >> b1;

	int b0 = x;
	
	return b16 + b8 + b4 + b2 + b1 + b0 + 1;
}
```

#### `unsigned floatPower2(int x)`

这个puzzle本🉑没有通过，不知道是什么原因导致在测试时陷入了无限循环

但是依然在这里列出思路，以便后续的更正

首先，题目要求的是2.0^x^ ，我们需要根据x的范围和IEEE754标准进行分类和比对

|       x < -149        | 也就是超过了非规格数能表示的最小的2的幂 | 在非规格数的格式下，2的幂由frac决定，frac最小为0x000001即为2^-23^，故能表示的最小的2的幂为2^-126-23^ 即2^-149^ |
| :-------------------: | --------------------------------------- | ------------------------------------------------------------ |
| **-149 <= x <= -126** | **用非规格数进行表示**                  |                                                              |
|  **-126 < x <= 127**  | **用规格数进行表示**                    | **在规格化下，2.0的浮点表示中exp=0b1000 0000，要进行幂运算，只需要在exp上加上相应的指数，也就是exp=127 + x，其余的位都是0** |
|      **x > 127**      | **第三类表示法**                        | **超过了能表示的最大值，返回0x7f800000**                     |

####  评分一览

```bash
Correctness Results     Perf Results
Points  Rating  Errors  Points  Ops     Puzzle
1       1       0       2       7       bitXor
1       1       0       2       1       tmin
1       1       0       2       10      isTmax
2       2       0       2       10      allOddBits
2       2       0       2       2       negate
3       3       0       2       11      isAsciiDigit
3       3       0       2       7       conditional
3       3       0       2       15      isLessOrEqual
4       4       0       2       9       logicalNeg
4       4       0       2       32      howManyBits
4       4       0       2       16      floatScale2
4       4       0       2       25      floatFloat2Int
0       0       0       0       0
0       0       0       0       0       10
0       4       1       0       9       floatPower2

Score = 56/62 [32/36 Corr + 24/26 Perf] (154 total operators)
```

