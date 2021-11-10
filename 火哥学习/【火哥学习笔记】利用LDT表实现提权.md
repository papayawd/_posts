

利用 LDT表中的段描述符不容易被软件查出来

```cpp
#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
char GDTaddr[6]={0};
char LDTtable[0x3ff] = {0};
__declspec(naked) void test()
{
	__asm
	{
		pushad;
		pushfd;
		lea eax,[GDTaddr+2]; 
		mov eax,[eax];// 以上两句讲gdt首地址放进eax
		lea eax,[eax+0x90]; // 我们修改的目标是gdt偏移为0x90的位置
		lea ecx,LDTtable;
		mov bx,cx;
		shl ebx,0x10;
		mov bx,0x03ff; // 这里存放的是ldt表的大小
		mov dword ptr [eax],ebx;
		lea eax,[eax+4];
		shr ecx,0x10;
		mov byte ptr [eax],cl;
		mov byte ptr [eax+1],0xe2; // 0xe2表示这个描述符是一个ldt描述符
		mov byte ptr [eax+4],ch; // 以上在构造ldt的描述符
		mov ax,0x93;
		lldt ax; // 使用R3的权限 加载ldt表 因为局部描述符服务于该进程(R3) 合理
		popfd;
		popad;
		retf;
	}
}
int main()
{
	char buf[6] = {0,0,0,0,0x48,0}; // 使用我们手动修改的0x48位置的调用门
	char ldt[]={0};
	int b=0;
	*((int*)(LDTtable+0x8)) = 0x0000ffff;
	*((int*)(LDTtable+0xc)) = 0x00cff300;// 构造段描述符
	printf("%X\t,GDTaddr:%X\n",test,GDTaddr);
	system("pause");
	__asm
	{
		sgdt GDTaddr; // 取出gdt
		push fs;
		call fword ptr buf; // 调用门提权手法
		sldt ldt; // 加载一下ldt看是否修改成功
		pop fs;
		mov ax,0xf; // f(h) = 1111(b) 使用下标为1 权限为3 在ldt表中查找的段选择子
		mov ds,ax; // 使用
		mov b,0x10; // 尝试赋值 看是否成功 不报错就成功
	}
	printf("%X\n",b);
	system("pause");	
	return 0;
}
```

