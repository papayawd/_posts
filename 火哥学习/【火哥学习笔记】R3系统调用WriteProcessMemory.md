---
title: 【火哥学习笔记】R3系统调用WriteProcessMemory
date: 2021-07-16 16:57:48
tags: windows
---

​	<!-- more -->

被修改的程序

```c
#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include<process.h>
int value = 100;
int main()
{
	printf("%PID : %d\n",_getpid());
	printf("addr of value : %X\n",&value);
	printf("value : %d\n",value);
	system("pause");
	while(1) // 一直输出 不然每次启动的PID都不一样
	{
		printf("changed value : %d\n",value);
		system("pause");
	}
	return 0;
}
```

修改程序

```c
#include<stdio.h>
#include<stdlib.h>
#include<windows.h>

int main()
{
	char writedata[]={100,0,0,0};
	DWORD oldprot,number = 0;
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS,FALSE,432); // PID need be changed
	if(handle == NULL)
	{
		printf("open process error\n");
		system("pause");
		return 0;
	}
	
	VirtualProtectEx(handle, (LPVOID)0x426A30, 4, PAGE_EXECUTE_READWRITE, &oldprot);
/*
	if (!WriteProcessMemory(handle, (LPVOID)0x426A30, &writedata, 4, &number)){
		printf("write error %d %d\n", GetLastError(), number);
	} // API change success	
*/


	printf("%X,%X,%X,%X,%X\n",handle, (LPVOID)0x426A30, &writedata, 4, &number);
	system("pause");
	__asm
	{
		push edx; 
		lea edx,number // IDA逆向kernel32.dll看WriteProcessMemory发现根本不是push的number的地址 而是handle句柄的地址 但是真正返回NumberOfBytesWritten又是放在当前push的地址 有点搞不懂
		push edx
		push 4
		lea edx,writedata
		push edx
		push 0x426A30
		mov edx,handle
		push edx;     // push end
		push 0        // 通过跟进WriteProcessMemory得知 执行sysenter之前 esp+8 才是第一个参数 所以需要push一个0占位esp+4    而后边的call自动储存返回地址在esp+0
		mov eax,0x115 // system code
		mov edx,0x7FFE0300 
		call dword ptr [edx]
		pop edx;
		pop edx;
		pop edx;
		pop edx;
		pop edx;
		pop edx;
		pop edx; // pop end
	}
	//printf("%d %d\n", GetLastError(), number);

	VirtualProtectEx(handle, (LPVOID)0x426A30, 4, oldprot, &oldprot);
	system("pause");

	return 0;
}
```

