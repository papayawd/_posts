# ARM&C++算法还原原理

## 编译c代码为Android平台的汇编语言

通过Android Studio 下载NDK，里面的 tool-chain 中有llvm，将其bin添加在PATH之前就可以替换整体的clang。

```shell
Papaya-mbp 🐳 ~/Desktop/vscode/android 
1305 ➜ which clang                                                                    [~]
/Users/liuxingyu/Library/Android/sdk/ndk/23.1.7779620/toolchains/llvm/prebuilt/darwin-x86_64/bin/clang
```

直接编译会报错找不到头文件，需要制定编译目标平台，使用`-target`参数，比如

```shell
clang -target arm-linux-android21 a.c
```

这里的target 由 4 部分组成：`arch-vendor-os-env`

arch 为 CPU 框架，比如 arm64、x86_64

vender 为厂商名称，比如：apple															          	这个基本不用

os 为操作系统，比如：darwin、ios、macosx、linux-android

env 为系统环境，比如：simulator（模拟器）、macabi (Mac Catalyst)                 这个基本不用

编译 thumb 汇编指令需要加上`-mthumb`参数，不加该参数编译出来的是 arm 汇编指令

```shell
clang -target arm-linux-android21 a.c -S -mthumb -o a_thumb.s   # thumb
clang -target arm-linux-android21 a.c -S -o a_arm.s             # arm
clang -target aarch64-linux-android21 a.c -S -o a_armv8.s       # armv8
```



## ARM汇编

各种指令其实需要的时候再去查询即可，主要是函数传参、结构体、数组、与x86下有所不同

### 寄存器

#### arm32寄存器

R0 ~ R7 为未分组寄存器，其对应的物理寄存器是**唯一**的。

R8 ~ R14 为分组寄存器，其对应**不同的且不同数量**的物理寄存器。R13在ARM指令中常用作**堆栈指针SP**；R14称为**子程序链接寄存器LR**(Link Register)，一般储存 bl 跳转指令的返回地址；

R15为**程序计数器PC**， pc 类似于 x86 下的 eip ，不同的是 arm7 框架下的三级流水线结构，包括取指（fetch）、译码（decode）、执行（execute）三级，当程序正在执行第一条指令的时候，pc 应该指向**第三条**指令

![](\picture\15266298788626.jpeg)

此外，在ARM9中，采用了五级流水线结构，是在ARM7的三级流水线结构后面添加了两个新的过程。因此，指令的执行过程和取指过程还是相隔一个译码过程，因而PC还是指向当前指令随后的第三条指令。 

R16用作CPSR(CurrentProgram Status Register，当前程序状态寄存器)，CPSR可在任何运行模式下被访问，它包括条件标志位、中断禁止位、当前处理器模式标志位，以及其他一些相关的控制和状态位。

每一种运行模式下又都有一个专用的物理状态寄存器，称为SPSR(Saved Program Status Register，备份的程序状态寄存器)，当异常发生时，SPSR用于保存CPSR的当前值，从异常退出时则可由SPSR来恢复CPSR。
**由于用户模式和系统模式不属于异常模式，它们没有SPSR，当在这两种模式下访问SPSR，结果是未知的**

#### arm64（aarch64）寄存器

x0~x7：传递子程序的参数和返回值，使用时不需要保存，多余的参数用堆栈传递，64位的返回结果保存在x0中。
x8：用于保存子程序的返回地址，使用时不需要保存。
x9~x15：临时寄存器，也叫可变寄存器，子程序使用时不需要保存。
x16~x17：子程序内部调用寄存器（IPx），使用时不需要保存，尽量不要使用。
x18：平台寄存器，它的使用与平台相关，尽量不要使用。
x19~x28：临时寄存器，子程序使用时必须保存。
x29：帧指针寄存器（FP），用于连接栈帧，使用时必须保存。
x30：链接寄存器（LR），用于保存子程序的返回地址。
x31：堆栈指针寄存器（SP），用于指向每个函数的栈顶。
此外还有对应的`w[n]`寄存器（4字节），是对应`x[n]`寄存器（8字节）的低位。

### 内存访问与跳转

```asm
ldr r1, [sp]  @ 4字节读取   []符号与 x86 相同，仍然是访问地址符号
ldrb					@ 1字节读取
ldrh					@ 2字节读取

str r1, [sp]	@ 4字节写入
strb					@ 1字节写入
strh					@ 2字节写入

B							@ 强制跳转 == jmp
BL						@ 带返回跳转（LR储存返回地址） 类似call
BLX						@ 带返回和状态切换跳转
BX						@ 带状态切换跳转           状态切换指：thumb 与 arm 之间的切换
```

### 函数传参

规则：**前4个参数使用r0～r3寄存器传，超过4个点部分与x86一样使用堆栈传参**

### 函数调用&&返回

规则：**使用 bl 调用函数，类似x86的call，不同点是不会 push 返回地址，而是将返回地址存入到 lr 寄存器中；因此函数返回时候不使用 ret，而是直接使用跳转指令 bx lr ；栈帧 old ebp 不会 push 入栈，而是保存在r11寄存器中；若是有返回值，则放入到 r0 寄存器中**

```c
function('a', 10, 100, 100.11f, 1000, 1000.111); // 编译这句话 得到下面的arm汇编代码
```

```asm
main:
	.fnstart
@ %bb.0:
	.save	{r11, lr}
	push	{r11, lr}
	.setfp	r11, sp
	mov	r11, sp													@ 储存 old sp
	.pad	#24
	sub	sp, sp, #24
	mov	r0, #0
	str	r0, [r11, #-8]                  @ 4-byte Spill
	str	r0, [r11, #-4]
	mov	r1, sp
	ldr	r2, .LCPI1_0
	str	r2, [r1, #12]
	ldr	r2, .LCPI1_1
	str	r2, [r1, #8]					@ 以上4行  double 的赋值
	str	r0, [r1, #4]					@ r0 == 0   [r1+4] = 0   long long 的高位的确是0
	mov	r0, #1000							
	str	r0, [r1]							@ 以上3行 long long 赋值
	mov	r0, #97								@ 参数 1     char  -> 'a'
	mov	r1, #10								@ 参数 2     short -> 10
	mov	r2, #100							@ 参数 3     int   -> 100
	ldr	r3, .LCPI1_2					@ 参数 4  	 float -> 100.11
	bl	function              @ bl 不仅跳转到 function 地址 并且把返回地址放进了 lr
	
	@...
	
.LCPI1_0:
	.long	1083130083                      @ 0x408f40e3
.LCPI1_1:
	.long	1408749273                      @ 0x53f7ced9
.LCPI1_2:
	.long	1120417874                      @ 0x42c83852 == 100.11f
	
	@...
	
function:
	push	{r4, r5, r6, r7, r11, lr}  			@ 保存现场 r0~r3 为参数就不用保存了 
	@...
	mov r0 , ret_value										@ 保存返回值到r0
	pop	{r4, r5, r6, r7, r11, lr}					@ 还原现场
	bx	lr																@ lr用来保存返回地址

```

### 结构体与数组的使用

规则：**仍然是使用指针来进行引用和赋值。结构体也存在x86下的对齐特征**

### 全局变量的引用

规则：**使用add r2, pc, r2 的方法获取地址，其中pc = 当前指令地址 随后的第三条指令的位置**

```c
#include <stdio.h>
int value;
int main(){
	value = 10000;
}
```

```asm
@ arm7
	ldr	r2, .LCPI1_1
.LPC1_1:
	add	r2, pc, r2							@ 这里框架为arm7 arm模式 每条指令长度为4 因此pc = .LPC1_1 + 8 
															@ r2 = value - (.LPC1_1 + 8) + .LPC1_1 + 8 = value 的地址
	mov	r1, #1808
	orr	r1, r1, #8192						@ 这里的 mov r1, 10000 被优化了一下
	@ ...
.LCPI1_1:
	.long	value-(.LPC1_1+8)
```

### 位运算操作

```asm
	mvn r0, r0									@ ~ 取反
	and r0, r0, r1							@ & 与
	oor r0, r1, r1							@ | 或
	eor r0, r0, r1							@ ^ 异或
	asr r0, r0, 4								@ >> 右移
	lsl r0, r0, 4								@ << 左移
```



