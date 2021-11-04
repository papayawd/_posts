---
title: 【火哥学习笔记】APC&DPC相关
date: 2021-08-26 21:22:52
tags: windows
---

​	<!-- more -->

## 前言

之前提到的，使用IDT表将每个中断或异常与处理该中断或异常的服务例程联系起来。直观的体验就是中断或异常产生就立即停止当前代码去执行相关的服务例程，但是中断与中断之间也有优先级。

更具体内容见《WINDOWS内核原理与实现》p49

## IRQL 中断请求级别

Windows规定了一套软件中断优先级，政委中断请求级别（IRQL，Interrupt Request Level）。使用0~31的数值来表示IRQL，数值越大，优先等级越高。处理器在运行的时候总是有一个当前IRQL，如果遇到中断的IRQL小于等于当前级别，就屏蔽该中断，直到处理器的IRQL被降下来为止。IRQL=0表示普通线程，称为PASSIVE_LEVEL或者被动级别，它优先级最低可以被任何其他级别的中断打断。IRQL=1表示**异步过程调用（APC，Asynchronous Procedure Call）** ，称为APC_LEVEL。IRQL=2表示处理器正在做以下两件事情之一：一、正在进行线程调度，比如切换为新的线程；二、正在处理一个硬件中断的后半部分（不是很紧急的部分），在windows中，这被称为**延迟过程调用（DPC，Deferred Procedure Call）** 。

## APC 异步过程调用

APC是线程相关的例程，只能在特定的线程环境中被执行，因而也一定在特定的地址空间中被执行。因为级别高于线程本身的指令流，所以很适合于实现各种异步通知事件，例如I/O的完成通知可以用APC来实现。实际上APC在KiServiceExit函数里被执行，而KiServiceExit这个函数是系统调用、异常或中断返回到用户空间的必经之路；其次KiDeliverApc函数也执行APC，它是专门负责执行APC的函数。

先看看APC相关的结构体

```c
ntdll!_KAPC
   +0x000 Type             : Int2B
   +0x002 Size             : Int2B // 以上两个都被写死了 Type=0x12 Size=0x30
   +0x004 Spare0           : Uint4B
   +0x008 Thread           : Ptr32 _KTHREAD // 目标线程 （要被添加APC的线程）
   +0x00c ApcListEntry     : _LIST_ENTRY // _KAPC_STATE中链表所指的地址 -0xc 才能到_APC结构的首地址
   +0x014 KernelRoutine    : Ptr32     void // 指向一个函数（调用ExFreePoolWithTag 释放APC）
   +0x018 RundownRoutine   : Ptr32     void // 
   +0x01c NormalRoutine    : Ptr32     void // 用户APC总入口，或者是真正的内核APC函数
   +0x020 NormalContext    : Ptr32 Void  // 是用户APC时，这是真正的APC函数；是内核APC时，这是NULL
   +0x024 SystemArgument1  : Ptr32 Void // APC函数的参数
   +0x028 SystemArgument2  : Ptr32 Void // APC函数的参数
   +0x02c ApcStateIndex    : Char  // 挂那个队列，0 1 2 3
   +0x02d ApcMode          : Char // =0 内核APC    =1 用户APC
   +0x02e Inserted         : UChar // 该APC是否已经被挂入链表 挂入后置 1
       
ntdll!_KAPC_STATE
   +0x000 ApcListHead      : [2] _LIST_ENTRY // 指向_KAPC中 +0xc的位置 下标0:内核APC 和 下标1:用户APC
   +0x010 Process          : Ptr32 _KPROCESS // 线程所属或者挂靠的进程
   +0x014 KernelApcInProgress : UChar // 内核APC是否正在执行
   +0x015 KernelApcPending : UChar // 是否有正在等待执行的内核APC
   +0x016 UserApcPending   : UChar // 是否有正在等待执行的用户APC
       
nt!_KTHREAD
   +0x034 ApcState         : _KAPC_STATE
   +0x138 ApcStatePointer  : [2] Ptr32 _KAPC_STATE
   +0x14c SavedApcState    : _KAPC_STATE
   +0x165 ApcStateIndex    : UChar

```

APC的大致流程如下：

1.通过KeInitializeApc取初始化一个APC

2.调用KeInsertQueueApc->KiInsertQueueApc将APC挂入到某个线程里

3.在该线程执行了系统调用、异常或中断后返回到用户空间，检查是否有待执行的APC，有的话调用KiDeliverApc完成所有APC队列任务

具体细节分析都在idb内，但是分析不是很完全，可以结合wrk看程序流程

## DPC 延迟调用

DPC的IRQL等级为DISPATCH_LEVEL，高于PASSIVE_LEVEL和APC_LEVEL，因此它优先于任何一个线程相关的函数并且屏蔽了线程调度；同时又低于所有硬件中断。之所以成为延迟，是因为它往往被利用来执行一些相对于当前高优先级的任务（比如硬件中断）来说不那么紧急的事情，从而缩短处理器停留在高IRQL的时间。他的经典用法就是定时器的实现。

DPC的大致流程与APC相似，相关变量都储存在KPRCB结构体内

1.通过KeInitializeDpc取初始化一个DPC

2.调用KeInsertQueueDpc将DPC挂入到某个处理器里

3.每当CPU的运行级别从DISPATCH_LEVEL或以上降低到DISPATCH_LEVEL以下时，如果有扫描DPC请求队列的要求存在，内核就会扫描DPC请求队列并执行DPC函数。一般是KeLowerIrql函数直接导致触发DPC。同样DPC也有队列，直接循环判断并且执行。

可以结合wrk看程序流程

以下是DPC的定时器实现 驱动程序

```c
#include <ntddk.h>
KDPC dpc = { 0 };
KTIMER ktimer = { 0 };
LARGE_INTEGER duetime = { 0 };
VOID dpcCall(
	_In_ struct _KDPC* Dpc,
	_In_opt_ PVOID DeferredContext,
	_In_opt_ PVOID SystemArgument1,
	_In_opt_ PVOID SystemArgument2
)
{
	DbgPrint("I'm DPC \n");
	KeSetTimer(&ktimer, duetime, &dpc);// 继续定时器
}
VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint("卸载了\n");
	KeCancelTimer(&ktimer);
}
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	driver->DriverUnload = DriverUnload;
	DbgPrint("加载了\n");
	KeInitializeTimer(&ktimer);
	KeInitializeDpc(&dpc, dpcCall, NULL);
	duetime.QuadPart = -30 * 1000 * 1000;
	KeSetTimer(&ktimer, duetime, &dpc); // 定时器
	return STATUS_SUCCESS;
}
```

