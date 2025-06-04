# CSAPP

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

### Phase_1

因为第一次做这样的lab所以做了参考，没有独立完成这一阶段的lab

参考：[官方writeup](https://csapp.cs.cmu.edu/3e/attacklab.pdf) [arthals‘ink博客](https://arthals.ink/blog/attack-lab#phase-1)

### Phase_2

编写一个程序放在缓存区，程序需要对%rdi赋值然后跳转到`touch2()`，需要将它的地址压入栈然后返回即可跳转，另外利用缓存区溢出将原来的return地址替换为缓存区地址，这样就可以执行我们存在缓存区的程序了

### Phase_3

我没有自己做出来，但是还是记录一下

正常的工作流应该是：

- 修改第二阶段的答案使得程序进入`touch3()`进行测试，看调用`hexmatch()`前后的缓存区变化情况来判断该函数对缓存区的覆盖范围
- 然后将我们的字符串放到不会被影响的缓存区区域
- 最后编写汇编码进行测试
  - 先将`touch3()`地址入栈
  - 然后将 **我们的字符串地址**传入`%rdi`中
  - 最后返回即可

- 然后按照上面的得到的地址信息去组织输入字符串即可

但是我遇到了一个问题，也就是在确定答案前进行测试的时候，我遇到了段错误，也就是说我没有办法利用gdb观察缓存区域的变化，导致我没做出来。

另一种思路：来自[b站](【深入理解计算机系统 attacklab-哔哩哔哩】 https://b23.tv/WyDNvzY) up主的评论：

```
回复 @雨随风来雨落去 :你看下hexmatch的代码，cbuf数组其实就是getbuf的栈帧，s字符串的起始地址是cbuf的首地址加上一个随机数对100求余剩下的余数，余数范围0到99，s字符串本身占用8字节，那么，cbuf数组可能0-107字节这个范围都可能被s字符串覆盖，只剩下最后两个字节是不会被覆盖的，但是我们要存的字符数组长8字节，剩下两个字节不够用，所以只能放到test的栈帧里了。
```

