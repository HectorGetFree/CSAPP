# CSAPP：程序员视角下的计算机系统

## 总结

> ​	期末周赶考+能力优先，所以匆忙地跟着参考敲了一遍最后几个lab。难确实是难，因为我并没有一些系统的实现思路，对优化就更无从下手了。尽管如此，看着优秀的实现敲一遍还是加深了对知识的理解。

​	言归正传，毕竟书还是一点一点啃下来的，既然读了书，所以就稍微总结一下，给没有读/想要读书的同学一些参考。

- 第一章：计算机系统漫游

  其实也是整本书的一个框架概览

- 第二章：信息的表示和处理

  这一章讲的就是计算机中**位运算，整数表示（有符号和无符号）及运算，浮点表示及运算（IEEE754）** -- 对应了 **datalab**

- 第三章：程序的机器级表示

  介绍了x86-64体系下的汇编，将高级语言跟汇编进行对照 -- 对应的是 **bomblab** **attacklab**

- 第四章：处理器体系结构

  介绍了Y86-64指令集，以及相应CPU的流水线实现，其实就是压缩了计算机组成的内容 -- 对应 **archlab**

- 第五章：优化程序性能

  如题，给了一些优化性能的方式 -- 在 **archlab**中有体现

- 第六章：存储器层次架构

  主要还是介绍存储层级结构和缓存实现的原理，并给出了本书封面中的 **存储器山** -- 对应 **cachelab**

- 第七章：链接

  讲述静态链接和动态链接的原理 

- 第八章：异常控制流

  虽然标题是异常控制流，但是其实感觉是拿他为引子，讲述了计算机中各种控制信号的处理，还有进程的知识 -- 对应 **shelllab**

- 第九章：虚拟内存

  介绍虚拟内存技术的原理，以及动态内存分配 -- 对应 **malloclab**

- 第十章：系统级I/O

  如题，介绍了底层I/O和健壮的RIO还有重定向等知识

- 第十一章：网络编程

  介绍网络编程的基本概念，给出了一个简单的服务器实现

- 第十二章：并发编程

  介绍三种形式的并发编程：基于进程的并发编程、基于I/O多路复用的并发编程以及基于线程的并发编程，还有信号量等知识，给出了一个可以处理并发的服务器实现

  最后三章共同对应 **proxylab**

## 推荐

​	如果你学完了UCB的**CS61ABC**三门课，在进入其他课程之前，我很推荐你读一读CSAPP。

​	其实这门课讲的内容跟**CS61C**很大一部分是重合的，因为相应的概念61C都会提到，为什么还推荐CSAPP，是因为我个人觉得CSAPP对概念的讲述更加紧扣，也更佳严谨和细致，所以我推荐学

​	拿浮点数表示举例，61C只会告诉你IEEE754标准是什么，但是CSAPP会告诉你为什么这样规定能更精妙的更平滑的在规格化和非规格化之见过渡。这样的例子还有很多

## 个人体验

​	本人实在才疏学浅，没办法将本书吃的特别透彻，lab也是没办法独立完成。尽管如此，我依然很高兴学了这门课，他使我对整个计算机系统有了一个理解的大框架，通过学习lab代码，我也知道了cache，malloc等实现的底层细节，甚至gdb也用的更加熟练。所以很感谢这门课程

## 个人lab记录

​	虽然很多都没有自己实现，但是还是整理、补充了很多对优秀实现的理解，包括我自己的体验，都记录在下面。

## data-lab

未独立完成的lab部分思路记录

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

## bomblab

常见GDB命令以及本lab需要用的命令汇总在[这个博客](https://arthals.ink/blog/bomb-lab#%E9%98%85%E8%AF%BB%E6%BA%90%E7%A0%81)的第一部分，该博客的剩余部分是北大ICS的lab实现

实现思路介绍

⚠️：应先自己尝试

### phase_1

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
  40135f:	84 c0                	test   %al,%al				#%al是空，说明比较完毕，则将%eax设置为0
  401361:	74 25                	je     401388 <strings_not_equal+0x50>
  401363:	3a 45 00             	cmp    0x0(%rbp),%al 	#%al不为空，那就跟（%rbx）比较，此时已经可以看出，标准字符串储存在%rbp中，相等就循环比较剩余部分，不相等直接设置结果退出函数
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

### phase_2

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
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)				#（%rsp）的值只会是1，不然会引爆炸弹
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	call   40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax 		#%eax存储的是%rbx地址处的上一个int
  400f1a:	01 c0                	add    %eax,%eax					#乘2
  400f1c:	39 03                	cmp    %eax,(%rbx)				# 比较2倍的%rbx地址处的上一个int 和 %rbx指向的int
  400f1e:	74 05                	je     400f25 <phase_2+0x29> 		# 相等就循环，不等直接爆
  400f20:	e8 15 05 00 00       	call   40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx 					# %rbx指向下一个int
  400f29:	48 39 eb             	cmp    %rbp,%rbx 					# 比较%rbx和%rbp作为循环条件
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx 		#从这里也可以看出是6个数
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

### Phase_3

这一阶段其实就是输入两个数字，第一个数字决定了switch语句的case，第二个数字用于判断是否成功对应了相应的case，答案不统一，对应上即可

```assembly
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	call   400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax 								#%eax中储存了scanf读取成功的个数
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>		#要求至少读入2个数(实测多输入几个数字也无所谓)
  400f65:	e8 d0 04 00 00       	call   40143a <explode_bomb>		#否则引爆
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)						#读进去的两个数分别位于%rsp+0x8的低4字节和高4字节
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>		#要求第一个数不能大于7，否则引爆
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax						#将第一个数传入%eax中
  400f75:	ff 24 c5 70 24 40 00 	jmp    *0x402470(,%rax,8)				#通过跳转表进行对应switch的case
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax								
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	call   40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	call   40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	ret
```

拿输入为`6 682`举例

第一个数字为6，那就跳转到第6个case（索引从1开始），然后`mov    $0x2aa,%eax `将%eax设置为0x2aa也就是10进制的682（注意：**输入的时候要输入10进制若输入0x2aa会由于格式不正确导致引爆**）,然后跳转、比较

`cmp    0xc(%rsp),%eax`这里的0xc(%rsp)的低4个字节其实就是我们输入的第二个数

### Phase_4

主体逻辑很明显，输入两个数，经过func4运算结果应该为0，且第二个输入的数字也要为0

```assembly
000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  401024:	e8 c7 fb ff ff       	call   400bf0 <__isoc99_sscanf@plt>
  401029:	83 f8 02             	cmp    $0x2,%eax
  40102c:	75 07                	jne    401035 <phase_4+0x29>   # 不等于2直接爆炸
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)          # 将输入的第一个数字跟0xe比较
  401033:	76 05                	jbe    40103a <phase_4+0x2e>   # 大于就炸
  401035:	e8 00 04 00 00       	call   40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx               
  40103f:	be 00 00 00 00       	mov    $0x0,%esi
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi
  401048:	e8 81 ff ff ff       	call   400fce <func4>
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c>
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  401056:	74 05                	je     40105d <phase_4+0x51>
  401058:	e8 dd 03 00 00       	call   40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	ret
```

传入func的参数是14和0以及我们输入的第一个数

我的做法是观察到当输入的一个数为7是func4正好返回0

然后通过GPT得到C语言版本如下：

```C
int func4(int target, int low, int high) {
    if (low > high) {
        return 0;
    }

    int mid = (high - low) / 2 + low;

    if (target < mid) {
        return 2 * func4(target, low, mid - 1);
    } else if (target > mid) {
        return 2 * func4(target, mid + 1, high) + 1;
    } else {
        return 0;
    }
}
```

其实就是一个二分查找返回路径编码，只有当target为7是才会返回0

补：可能有其他答案

### Phase_5

字符串解码问题，根据phase_1的经验我们很容易得到标准字符串，但是phase_5的代码对输入字符串进行了加密，如何进行解密？我的做法是 **暴力枚举**，由于没有找到合理的加密规律，我选择了对字符一一进行加密得到对应的字符，然后拼接成加密后可以得到标准字符串的输入字符串，最后成功解码

补：

后来在看b站的时候有了想法，应该只要是ascii码低4位是标准字符串对应的码值即可

### Phase_6

Phase_6看了半天没做出来。汇编码读懂了一部分，理解了输入1-6 一共6个数字，然后用7去减，卡在了链表那里，死活没弄明白链表怎么构建的，所以这里就不放解析了，相关解析可以移步到[b站](https://www.bilibili.com/video/BV1vu411o7QP?t=0.6&p=7)

## attacklab

好吧，这个lab没能独立做出来，原因是对栈溢出和ROP理解不是很到位，也没有对`ret push pop`这些指令特别熟悉。总的来说就是对栈的行为都不太熟🙂‍↕️

根据参考下来的文档[官方writeup](https://csapp.cs.cmu.edu/3e/attacklab.pdf)， [arthals‘ink博客](https://arthals.ink/blog/attack-lab#phase-1)，[b站](https://b23.tv/WyDNvzY)

再梳理一下工作流吧

**前三个阶段是通过代码注入来攻击程序**

先执行以下命令

```bash
# 将ctarget反汇编
objdump -d ctarget > ctarget.asm
```

### Phase_1

本阶段要求当`getbuf()`结束时跳转到`touch1()`，我们需要这样做

- 查看`ctarget.asm`中的`getbuf()`得到缓冲区大小

- 编写我们的字符串

  - 前面的缓冲区内随便填
  - 但是超出缓冲区4个字节的部分写上`touch1()`的起始地址，注意要反着写（小端）

- 输入下面的命令即可

  `./hex2raw -i input.txt | ./ctarget -q`

  `input.txt`替换为你自己的字符串输入文件

### Phase_2

本阶段要求为参数寄存器赋值，将代码注入到缓冲区然后执行注入代码，即跳到`touch2()`，工作流如下：

- 编写汇编码，内容是：
  - 将`touch2()`的起始地址`push`到栈
  - 将`cookie`传递到`%rdi`寄存器
  - 返回`ret`
  - 原因是，我们通过压入一个地址，`ret`语句会从栈中取出这个地址然后跳转
- 然后输入命令
  - `gcc -c p2.s # 编译 `
  - `objdump -d p2.o > p2.d # 翻译为字节码,p2.d后缀无所谓`

- 通过`p2.d`文件得到指令的字节码

- 把这个字节码填入输入字符串的首部

  - 直接写不需要倒序（原因是汇编指令本身已经是**按字节序列排好**的机器码，逐字节读入即可）
  - 指令字节码之间不需要分隔

- 剩下的缓冲区填00即可

- 超出缓冲区的部分，我们用缓冲区的起始地址进行覆盖，这样才能跳转到我们的注入代码

  - 要获得缓冲区的起始地址，找到分配缓冲区空间后的栈指针即可

  - 使用gdb进行查看

  - ```bash
    gdb ctarget
    b getbuf
    r -q # 要带 -q否则会检测到宿主机名称
    ```

- 输入下面的命令即可

  `./hex2raw -i input.txt | ./ctarget -q`

  `input.txt`替换为你自己的字符串输入文件

### Phase_3

正常的工作流应该是：

- 修改第二阶段的答案使得程序进入`touch3()`进行测试，看调用`hexmatch()`前后的缓存区变化情况来判断该函数对缓存区的覆盖范围
- 然后将我们的字符串放到不会被影响的缓存区区域
- 最后编写汇编码进行测试
  - 先将`touch3()`地址入栈
  - 然后将 **我们的字符串地址**传入`%rdi`中
  - 最后返回即可

- 然后按照上面的得到的地址信息去组织输入字符串即可

但是我遇到了一个问题，也就是在确定答案前进行测试的时候，我遇到了段错误，也就是说我没有办法利用gdb观察缓存区域的变化，导致我没做出来。

另一种思路：来自[b站]( https://b23.tv/WyDNvzY) up主的评论：

```
回复 @雨随风来雨落去 :你看下hexmatch的代码，cbuf数组其实就是getbuf的栈帧，s字符串的起始地址是cbuf的首地址加上一个随机数对100求余剩下的余数，余数范围0到99，s字符串本身占用8字节，那么，cbuf数组可能0-107字节这个范围都可能被s字符串覆盖，只剩下最后两个字节是不会被覆盖的，但是我们要存的字符数组长8字节，剩下两个字节不够用，所以只能放到test的栈帧里了。
```

### **phase_4,phase_5**

这两个阶段利用的技术是 **ROP**

基本工作流是：

- 先执行`objdump -d rtarget > rtarget.asm`

- 在`rtarget.asm`中找到`<start_farm>`与`<end_farm>`，这之间的指令是我们可以去用的
- 然后确定你要执行的指令
- 对应表格找到相关指令的字节码
- 在`<start_farm>`与`<end_farm>`间检索可以你需要的指令（也就是你可以取相关指令字节码的一部分来满足你的需求），注意要检索那些以`c3`结尾的字节码，记录下指令的地址
- 然后填入输入字符串即可（注意小端，要倒着写）

篇幅有限，具体的还是参考[b站](https://b23.tv/WyDNvzY)，尤其是phase_5有些难度，指令跨度较大，寄存器较多

## archlab

这是一个有关Y86处理器架构的lab（来自课本的第4章），一共A,B,C三个部分，我只试着做了做前两个部分，最后一个部分不愿意做了，主要是因为感觉自己对汇编优化这块不太敏感，做这个没什么意思。但是还是梳理一下工作流

### Part A

先进入`sim/misc`目录执行`make`命令（注：我在make时遇到了变量重复定义的问题，我不知道如何解决，从而导致一些文件无法被make生成，目前的解决方法是：在`sim/misc/yis.h`中为`int lineno;`加上`extern`这样你可以`make`出partA需要的文件来进行测试，但是PartB我还没有解决）

这一部分要求你写三个y86汇编程序

`sum.ys`：对一个链表进行求和

`rsum.ys`:`sum.ys`的递归版本

`copy.ys`:将一个一个字块复制到内存中的另一个字块，应计算所有字块中值的校验和`xor`

参考的C语言版本在`sim/misc/example.c`中

大致的代码格式如下：

```y86
    .pos 0	# 位置指示符，用来指定接下来的代码或数据在内存中的绝对地址。
    irmovq stack, %rsp	# 设置栈指针指向stack地址
    call main	# 调用main函数
    halt	# 程序结束


    .align 8	# 字对齐
ele1:	#链表
    .quad 0x00a
    .quad ele2
ele2:
    .quad 0x0b0
    .quad ele3
ele3:
    .quad 0xc00
    .quad 0
    
main:
    irmovq ele1, %rdi
    call list_sum
    ret

list_sum:
    pushq %rbp # 保存rbp
    xorq %rax, %rax # 清零
    jmp test

loop:
    mrmovq (%rdi), %rsi 
    addq %rsi, %rax
    mrmovq 8(%rdi), %rdi
    jmp test

test:
    andq %rdi, %rdi
    jne loop
    popq %rbp
    ret

    .pos 0x200 # 在内存地址 0x200 处定义标签 stack
stack:	 # 栈底
```

执行`./yas xxx.ys && ./yis xxx.yo`命令即可得到输出，正确的输出如下

```
Stopped in 31 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rsp:   0x0000000000000000      0x0000000000000200
%rsi:   0x0000000000000000      0x0000000000000c00

Changes to memory:
0x01f0: 0x0000000000000000      0x000000000000005b
0x01f8: 0x0000000000000000      0x0000000000000013
```

看%rax即可判断是否得到正确的结果

有关内存的就看下面半段，冒号前是地址，冒号后前半段是执行前的值，后半段是执行后的值

### Part B

在`sim/seq`目录下，查看`seq-full.hcl`根据注释判断你是否要把`iadd`填入相应的部分即可

### Part C

请参考 [博客](https://arthals.ink/blog/arch-lab#partc), 该博客也是我在做这个lab时的参考

## cachelab

### Part-A

要求模拟缓存，我对着参考一点一点把代码敲上去的，只能说完全看懂了，但是让我自己做还是有些困难，然后给每一步都添加了详尽的注释

### Part-B

#### 32 * 32：

每一行只能存8个int，为了避免多次读取/写入导致覆盖和miss，我们采用 8 * 8分块的思路，具体原理参考 [这里](https://arthals.ink/blog/cache-lab#partb) 

尽管做了参考，还是有些地方不好理解，所以在下面进行记录

需要注意的是，在优化版本中，不能将A中数据全部写入B后再对B进行转置，这要考虑A写入B时的数据顺序，对于A来讲，他的8*8块是从左到右从上到下进行移动的，而对B来讲他的则是从上到下然后从左到右进行移动的，所以不能对B整体进行转置，应该在 8 * 8小块中进行转置，这样的话结果才会是正确的

还要注意的是：参考里的博主很细心的分析了到底什么时候才后发生冲突，需要理解学习

#### 64 * 64:

总体思路就是借区域，跟着[这里](https://zhuanlan.zhihu.com/p/387662272)的图示把代码敲了下来，原理都标注在注释，没有进一步优化，只完成了最基本的部分。然而还是有很多坑，比方说你在读取了这段数据之后，要写就得立即写，访问了另一部分的数据再回来写miss肯定就增多了。

#### 61 * 67:

利用分块技术，由于无法进行偶数对其，只能不断尝试分块大小得到结果，试验结果如下：

| n    | Miss | n    | Miss |
| ---- | ---- | ---- | ---- |
| 2    | 3116 | 11   | 2090 |
| 3    | 2649 | 12   | 2058 |
| 4    | 2626 | 13   | 2049 |
| 5    | 2297 | 14   | 1997 |
| 6    | 2225 | 15   | 2022 |
| 7    | 2153 | 16   | 1993 |
| 8    | 2119 | 17   | 1951 |
| 9    | 2093 | 18   | 1962 |
| 10   | 2077 | 19   | 1980 |

可以观察到当分块大小为17*17是miss最小，而且满足题目要求的miss<2000

长宽不等的矩阵应该是最实际的情况

## shelllab

参考：[b站郭郭wg](https://b23.tv/3KCTb48)

(最后不知道如何通过测试，因为运行相关命令后也没有输出>tsh这样的字样等待输入，对照着代码好像逻辑也没问题，就先搁置）

这个lab的关键是你要清楚每个要实现的函数是干什么的，以及相关信号的处理`handler()`，还有bg fg处理的机制，如何等待正确等待fg（spin）以及对信号的阻塞以及复原`sigprocmask()`

具体的实现逻辑我都加在代码注释了

## malloclab

参考：[Arthals’ ink](https://arthals.ink/blog/malloc-lab)

考察对动态内存分配的理解，关键是要理解如何组织你的堆结构

参考着博客做了下来，有关代码的理解都加在注释里了

## proxylab

参考：[Arthals’ ink](https://arthals.ink/blog/proxy-lab)

要求实现一个代理服务器

相关代码来自参考，并在跟着敲代码的时候增添了一些自己的注释
