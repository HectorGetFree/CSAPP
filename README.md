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

### bomblab

常见GDB命令以及本lab需要用的命令汇总在[这个博客](https://arthals.ink/blog/bomb-lab#%E9%98%85%E8%AF%BB%E6%BA%90%E7%A0%81)的第一部分，该博客的剩余部分是北大ICS的lab实现

实现思路介绍

⚠️：应先自己尝试

#### phase_1

```assembly
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	call   401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	call   40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	ret
```

phase_1没有那么难，核心代码就是进入到`strings_not_equal`，这个函数会判断输入字符串和标准字符串是否一致，根据返回值来决定炸弹是否引爆。**因而我们只需要找到标准字符串所在的地址，然后打印出来即可**

```assembly
0000000000401338 <strings_not_equal>:
  401338:	41 54                	push   %r12
  40133a:	55                   	push   %rbp
  40133b:	53                   	push   %rbx
  40133c:	48 89 fb             	mov    %rdi,%rbx
  40133f:	48 89 f5             	mov    %rsi,%rbp
  401342:	e8 d4 ff ff ff       	call   40131b <string_length>
  401347:	41 89 c4             	mov    %eax,%r12d
  40134a:	48 89 ef             	mov    %rbp,%rdi
  40134d:	e8 c9 ff ff ff       	call   40131b <string_length>
  401352:	ba 01 00 00 00       	mov    $0x1,%edx
  401357:	41 39 c4             	cmp    %eax,%r12d
  40135a:	75 3f                	jne    40139b <strings_not_equal+0x63>
  40135c:	0f b6 03             	movzbl (%rbx),%eax
  40135f:	84 c0                	test   %al,%al #%al是空，说明比较完毕，则将%eax设置为0
  401361:	74 25                	je     401388 <strings_not_equal+0x50>
  401363:	3a 45 00             	cmp    0x0(%rbp),%al #%al不为空，那就跟（%rbx）比较，此时已经可以看出，标准字符串储存在%rax中，相等就循环比较剩余部分，不相等直接设置结果退出函数
  401366:	74 0a                	je     401372 <strings_not_equal+0x3a>
  401368:	eb 25                	jmp    40138f <strings_not_equal+0x57>
  40136a:	3a 45 00             	cmp    0x0(%rbp),%al
  40136d:	0f 1f 00             	nopl   (%rax)
  401370:	75 24                	jne    401396 <strings_not_equal+0x5e>
  401372:	48 83 c3 01          	add    $0x1,%rbx
  401376:	48 83 c5 01          	add    $0x1,%rbp
  40137a:	0f b6 03             	movzbl (%rbx),%eax
  40137d:	84 c0                	test   %al,%al
  40137f:	75 e9                	jne    40136a <strings_not_equal+0x32>
  401381:	ba 00 00 00 00       	mov    $0x0,%edx
  401386:	eb 13                	jmp    40139b <strings_not_equal+0x63>
  401388:	ba 00 00 00 00       	mov    $0x0,%edx
  40138d:	eb 0c                	jmp    40139b <strings_not_equal+0x63>
  40138f:	ba 01 00 00 00       	mov    $0x1,%edx
  401394:	eb 05                	jmp    40139b <strings_not_equal+0x63>
  401396:	ba 01 00 00 00       	mov    $0x1,%edx
  40139b:	89 d0                	mov    %edx,%eax
  40139d:	5b                   	pop    %rbx
  40139e:	5d                   	pop    %rbp
  40139f:	41 5c                	pop    %r12
  4013a1:	c3                   	ret
```

进入到这个equal函数，先得到标准字符串和输入字符串的长度，长度不一样直接`%rax` 设置为`%edx`, 也就是1；

`401357:	41 39 c4             	cmp    %eax,%r12d`

很显然，这两个长度分别存储在`%eax 和 %r12d`中

然后`40135c:	0f b6 03             	movzbl (%rbx),%eax`将字符串中的一个字符移动到`%eax`中 

进行比较即可（见注释）

#### phase_2

Phase_2其实不需要理解全部代码，特别是`read_six_numbers`中的代码，因为大部分都是对内存的管理，方便读入数字

如你所见，有``read_six_numbers``说明该阶段要读入6个数字（当你读了汇编码之后发现确实是这样）

重点思路标注在注释里

```assembly
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp
  400f02:	48 89 e6             	mov    %rsp,%rsi
  400f05:	e8 52 05 00 00       	call   40145c <read_six_numbers>
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp) # （%rsp）的值只会是1，不然会引爆炸弹
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	call   40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax #%eax存储的是%rbx地址处的上一个int
  400f1a:	01 c0                	add    %eax,%eax#乘2
  400f1c:	39 03                	cmp    %eax,(%rbx)# 比较2倍的%rbx地址处的上一个int 和 %rbx指向的int
  400f1e:	74 05                	je     400f25 <phase_2+0x29> # 相等就循环，不等直接爆
  400f20:	e8 15 05 00 00       	call   40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx # %rbx指向下一个int
  400f29:	48 39 eb             	cmp    %rbp,%rbx # 比较%rbx和%rbp作为循环条件
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx #从这里也可以看出是6个数
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp 
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
  400f3c:	48 83 c4 28          	add    $0x28,%rsp
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	ret
```

根据注释分析，我们要输入的是一个6项等比数列，比为2，由于循环前的比较时%rsp的值只会是1，且 第一次循环有：

`lea    0x4(%rsp),%rbx` `	mov    -0x4(%rbx),%eax`

故该数列的首相为1

即可得到答案
