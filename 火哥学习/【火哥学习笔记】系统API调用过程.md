

## sysenter 快速系统调用



拿`WriteFile`函数为例，因为外层函数有很多检查代码，看起来复杂，这里直接展示`NtWriteFile`

```c++
.text:7C92DF60 ; __stdcall NtWriteFile(x, x, x, x, x, x, x, x, x)
.text:7C92DF60                 public _NtWriteFile@36
.text:7C92DF60 _NtWriteFile@36 proc near               ; CODE XREF: RtlGetSetBootStatusData(x,x,x,x,x,x)+CB↓p
.text:7C92DF60                                         ; RtlCreateBootStatusDataFile()+112↓p ...
.text:7C92DF60                 mov     eax, 112h       ; NtWriteFile
.text:7C92DF65                 mov     edx, 7FFE0300h
.text:7C92DF6A                 call    dword ptr [edx]
.text:7C92DF6C                 retn    24h ; '$'
.text:7C92DF6C _NtWriteFile@36 endp
```

这里将系统调用号`0x112`放进了`eax`，并且这之后以及到R0之后，不会覆盖`eax`的值，也不会使用`eax`寄存器做其他事（只会用来判断以及提取SSTD表的索引）

`R3`的线性地址`0x7FFE0000`与R0的线性地址`0xffdf0000`共享一张物理页，里面储存了一个名称为`_KUSER_SHARED_DATA`的结构 如下

```cpp
kd> dt _KUSER_SHARED_DATA 0xffdf0000
ntdll!_KUSER_SHARED_DATA
   +0x000 TickCountLow     : 0x6590
   +0x004 TickCountMultiplier : 0xfa00000
   +0x008 InterruptTime    : _KSYSTEM_TIME
   +0x014 SystemTime       : _KSYSTEM_TIME
   +0x020 TimeZoneBias     : _KSYSTEM_TIME
   +0x02c ImageNumberLow   : 0x14c
   +0x02e ImageNumberHigh  : 0x14c
   +0x030 NtSystemRoot     : [260] 0x43
   +0x238 MaxStackTraceDepth : 0
   +0x23c CryptoExponent   : 0
   +0x240 TimeZoneId       : 0
   +0x244 Reserved2        : [8] 0
   +0x264 NtProductType    : 1 ( NtProductWinNt )
   +0x268 ProductTypeIsValid : 0x1 ''
   +0x26c NtMajorVersion   : 5
   +0x270 NtMinorVersion   : 1
   +0x274 ProcessorFeatures : [64]  ""
   +0x2b4 Reserved1        : 0x7ffeffff
   +0x2b8 Reserved3        : 0x80000000
   +0x2bc TimeSlip         : 0
   +0x2c0 AlternativeArchitecture : 0 ( StandardDesign )
   +0x2c8 SystemExpirationDate : _LARGE_INTEGER 0x0
   +0x2d0 SuiteMask        : 0x110
   +0x2d4 KdDebuggerEnabled : 0x3 ''
   +0x2d5 NXSupportPolicy  : 0x2 ''
   +0x2d8 ActiveConsoleId  : 0
   +0x2dc DismountCount    : 0
   +0x2e0 ComPlusPackage   : 0xffffffff
   +0x2e4 LastSystemRITEventTickCount : 0x53b2c
   +0x2e8 NumberOfPhysicalPages : 0x1ff6a
   +0x2ec SafeBootMode     : 0 ''
   +0x2f0 TraceLogging     : 0
   +0x2f8 TestRetInstruction : 0xc3
   +0x300 SystemCall       : 0x7c92e4f0 //    入口点
   +0x304 SystemCallReturn : 0x7c92e4f4
   +0x308 SystemCallPad    : [3] 0
   +0x320 TickCount        : _KSYSTEM_TIME
   +0x320 TickCountQuad    : 0
   +0x330 Cookie           : 0xff08deac

```

而`NtWriteFile`中`call [0x7FFE0300]` 则是`+0x300`偏移处的 `SystemCall  : 0x7c92e4f0` 

```cpp
kd> u 0x7c92e4f0
ntdll!KiFastSystemCall:
7c92e4f0 8bd4            mov     edx,esp // 将参数首地址让进edx，0环中用
7c92e4f2 0f34            sysenter   
ntdll!KiFastSystemCallRet:
7c92e4f4 c3              ret
```

`sysenter`指令会用到rdmsr寄存器内的值，rdmsr是一个大的寄存器，通过索引可以查询里面的值，这里用到的是

```cpp
kd> rdmsr 174
msr[174] = 00000000`00000008 //cs0 选择子
kd> rdmsr 175
msr[175] = 00000000`f88c7000 // esp0 但是好像没有用到
kd> rdmsr 176
msr[176] = 00000000`805426e0 // eip0 （KiFastCallEntry）
//--------------------------------------------------
kd> u 805426e0
nt!KiFastCallEntry:
805426e0 b923000000      mov     ecx,23h
805426e5 6a30            push    30h
805426e7 0fa1            pop     fs
805426e9 8ed9            mov     ds,cx
805426eb 8ec1            mov     es,cx
805426ed 648b0d40000000  mov     ecx,dword ptr fs:[40h]
805426f4 8b6104          mov     esp,dword ptr [ecx+4]
805426f7 6a23            push    23h
// 这里就是R3到R0的代码了
```

`KiFastCallEntry`函数中具体汇编的逆向可以见[ntoskrnl.exe.idb]()内的注释 大概就是修改为`R0`的环境（`EIP`，`CS`，`ESP`，`SS`）

这里的`esp`操作之后维护了一个`_KTRAP_FRAME`的结构 结构如下

```cpp
ntdll!_KTRAP_FRAME
   +0x000 DbgEbp           : Uint4B
   +0x004 DbgEip           : Uint4B // 这两个寄存器同R3保存的 （分析代码得知
   +0x008 DbgArgMark       : Uint4B
   +0x00c DbgArgPointer    : Uint4B
   +0x010 TempSegCs        : Uint4B
   +0x014 TempEsp          : Uint4B // 这里是R0的Cs、Esp
   +0x018 Dr0              : Uint4B
   +0x01c Dr1              : Uint4B
   +0x020 Dr2              : Uint4B
   +0x024 Dr3              : Uint4B
   +0x028 Dr6              : Uint4B
   +0x02c Dr7              : Uint4B // 调试寄存器，与硬件断点有关 分析文件中有提到如何让硬件断点失效
   +0x030 SegGs            : Uint4B // 这里以下的基本是R3的数据
   +0x034 SegEs            : Uint4B
   +0x038 SegDs            : Uint4B
   +0x03c Edx              : Uint4B
   +0x040 Ecx              : Uint4B
   +0x044 Eax              : Uint4B
   +0x048 PreviousPreviousMode : Uint4B
   +0x04c ExceptionList    : Ptr32 _EXCEPTION_REGISTRATION_RECORD
   +0x050 SegFs            : Uint4B
   +0x054 Edi              : Uint4B
   +0x058 Esi              : Uint4B
   +0x05c Ebx              : Uint4B
   +0x060 Ebp              : Uint4B
   +0x064 ErrCode          : Uint4B
   +0x068 Eip              : Uint4B
   +0x06c SegCs            : Uint4B
   +0x070 EFlags           : Uint4B
   +0x074 HardwareEsp      : Uint4B
   +0x078 HardwareSegSs    : Uint4B
   +0x07c V86Es            : Uint4B
   +0x080 V86Ds            : Uint4B
   +0x084 V86Fs            : Uint4B
   +0x088 V86Gs            : Uint4B // 以上四个 虚拟8086模式用到
// 这里不一定每个值都用到了，根据具体的代码具体分析
```

## int 2e 中断门系统调用

在windows2000之后的版本都没有用到这个方法执行系统调用了，但是还是保存了对应的函数

```cpp
.text:7C92E500 ; _DWORD __stdcall KiIntSystemCall()
.text:7C92E500                 public _KiIntSystemCall@0
.text:7C92E500 _KiIntSystemCall@0 proc near            ; DATA XREF: .text:off_7C923428↑o
.text:7C92E500
.text:7C92E500 arg_4           = byte ptr  8
.text:7C92E500
.text:7C92E500                 lea     edx, [esp+8]  // 参数首地址放近edx，R0用
.text:7C92E504                 int     2Eh             ; DOS 2+ internal - EXECUTE COMMAND
.text:7C92E504                                         ; DS:SI -> counted CR-terminated command string
.text:7C92E506                 retn
.text:7C92E506 _KiIntSystemCall@0 endp
```

`KiIntSystemCall`就可以类比`KiFastSystemCall`，这里的系统调用号仍然是放进`eax`寄存器

我们找一下`idt`表中的第0x2e项拆开看看

```cpp
kd> r idtr
idtr=8003f400
    
kd> dq 8003f400 + 2e*8
8003f570  8054ee00`00082611 80548e00`0008590c
    
kd> u 80542611
nt!KiSystemService:
80542611 6a00            push    0
80542613 55              push    ebp
80542614 53              push    ebx
80542615 56              push    esi
80542616 57              push    edi
80542617 0fa0            push    fs
80542619 bb30000000      mov     ebx,30h
8054261e 668ee3          mov     fs,bx
// 最后通过中断门执行KiSystemService函数 具体分析见分析文件（最后跳转到了sysenter进入的函数中
```





