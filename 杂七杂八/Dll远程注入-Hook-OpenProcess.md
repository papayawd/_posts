---
title: 'Dll远程注入[Hook OpenProcess]'
date: 2021-04-07 19:04:49
tags: windows
---

​	 <!-- more -->

通过远程注入Dll到任务管理器，实现任务管理器无法关闭notepad.exe的功能

注入原理是，像Kernel32这样的 程序必须的dll 被不同程序加载 偏移一般是相同的，我们通过获取注入程序Dll_Injection.exe的LoadLibraryA函数地址来确定目标进程的LoadLibraryA函数地址。然后通过给目标程序创建新的线程调用LoadLibraryA，把我们的MyDll.dll加载带目标进程中。而MyDll.dll中的主函数在dll被加载的时候，会带上ul_reason_for_call参数项为 DLL_PROCESS_ATTACH 调用，这时候dll的主函数中就可以实现我们想实现的东西，这里就是hook OpenProcess

功能原理是，任务管理器关闭notepad.exe的时候，需要通过OpenProcess获取其具柄，Hook了OpenProcess之后可以判断任务管理器是否要对notepad.exe执行操作，从而达到任务管理器无法关闭notepad.exe的目的

程序环境：winxp  VC++绿色版

代码参考：https://blog.csdn.net/zuishikonghuan/article/details/47781883

**[ 2021-8-25 ]补充：远程注入可以用更底层的APC的方式注入，具体操作就是给目标进程一个用户APC，APC的回调函数就是LoadLibnaryA，去加载我们的dll**



```c
// Mydll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <windows.h>
#include "tlhelp32.h"
#include "tchar.h"

DWORD addr_OpenProcess;
BYTE code[8]={0};
BYTE oldcode[8]={0};

DWORD GetpidByName(char* name)
{
	PROCESSENTRY32 mes; // "tlhelp32.h"
	HANDLE hShot;
	bool exist;
	DWORD pid = -1;
	hShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);// "tlhelp32.h"
	// CreateToolhelp32Snapshot函数
        // 参数1：要得到进程的信息，赋为TH32CS_SNAPPROCESS
        //        要得到线程的信息，赋为TH32CS_SNAPTHREAD
        //        要得到指定进程的堆列表，赋为TH32CS_SNAPHEAPLIST
        //        要得到指定进程的模块列表，赋为TH32CS_SNAPMODULE
        //        要获取其他标记代表意义，查看MSDN
        // 参数2：指定将要快照的进程ID  如果该参数为0表示快照当前进程
        //         该参数只有在设置了TH32CS_SNAPHEAPLIST或TH32CS_SNAPMOUDLE后才有效，在其他情况下该参数被忽略，所有的进程都会被快照。
        // 返回值：调用成功，返回快照的句柄，调用失败，返回INVAID_HANDLE_VALUE
	if(hShot == NULL) return -1;
	mes.dwSize = sizeof(mes); // 使用PROCESSENTRY32结构时，先设置其大小
	exist = Process32First(hShot,&mes); // 返回快照中第一个进程信息到mes   函数失败返回0
	while(exist)
	{
		// _tcsstr 功能同 strstr
		if(_tcsstr(mes.szExeFile,name) != NULL)
		{
			pid = mes.th32ProcessID; // 获取目标进程pid
			break;
		}
		exist = Process32Next(hShot,&mes);// 返回快照中下一个进程信息到mes   函数失败返回0
	}
	CloseHandle(hShot);
	return pid;
}

HANDLE WINAPI MyOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId){
	HANDLE handle;
	char* name = "notepad.exe";
	if (GetpidByName(name) == dwProcessId){
		SetLastError(5);
		return NULL;
	}
 
	DWORD old;
	if (VirtualProtectEx(GetCurrentProcess(), (void*)addr_OpenProcess, 5, PAGE_EXECUTE_READWRITE, &old)){
		WriteProcessMemory(GetCurrentProcess(), (void*)addr_OpenProcess, oldcode, 5, NULL);// 暂时还原之前的opcode 
		VirtualProtectEx(GetCurrentProcess(), (void*)addr_OpenProcess, 5, old, &old);
	}
  
	handle = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId); // 调用真正的OpenProcess 
  
	if (VirtualProtectEx(GetCurrentProcess(), (void*)addr_OpenProcess, 5, PAGE_EXECUTE_READWRITE, &old)){
		WriteProcessMemory(GetCurrentProcess(), (void*)addr_OpenProcess, code, 5, NULL);// 又修改为我们的MyOpenProcess
		VirtualProtectEx(GetCurrentProcess(), (void*)addr_OpenProcess, 5, old, &old);
	}
 
	return handle;
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: // 当程序刚加载该Dll的时候会触发该项 
		addr_OpenProcess = 0;
		HMODULE hDll;
		hDll = LoadLibrary(TEXT("Kernel32.dll")); // 获取Kernel32句柄
		addr_OpenProcess = (DWORD)GetProcAddress(hDll, "OpenProcess"); // 在Kernel32中查找Openprocess的地址
		if (addr_OpenProcess){
			code[0] = 0xe9; // call 的opcode
			DWORD a = (DWORD)MyOpenProcess - (DWORD)addr_OpenProcess - 5; // call之后的opcode  公式
			RtlMoveMemory(code + 1, &a, 4);  // DWORD转opcode  小端
 
			DWORD old;
			if (VirtualProtectEx(GetCurrentProcess(), (void*)addr_OpenProcess, 5, PAGE_EXECUTE_READWRITE, &old)){ // 修改虚拟内存保护属性 为可读可写 
				//  最后一个参数为 原访问方式 用于保存改变前的保护属性
				RtlMoveMemory(oldcode, (void*)addr_OpenProcess, 5); // 储存原来的opcode 
				WriteProcessMemory(GetCurrentProcess(), (void*)addr_OpenProcess, code, 5, NULL); // 覆盖为 “call MyOpenprocess”
				VirtualProtectEx(GetCurrentProcess(), (void*)addr_OpenProcess, 5, old, &old); // 还原原本的保护属性
			}
		}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}



    return TRUE;
}


```

```c
// Dll_Injection.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include "tlhelp32.h"
#include "tchar.h"


DWORD GetpidByName(char* name)
{
	PROCESSENTRY32 mes; // "tlhelp32.h"
	HANDLE hShot;
	bool exist;
	DWORD pid = -1;
	hShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);// "tlhelp32.h"
	// CreateToolhelp32Snapshot函数
        // 参数1：要得到进程的信息，赋为TH32CS_SNAPPROCESS
        //        要得到线程的信息，赋为TH32CS_SNAPTHREAD
        //        要得到指定进程的堆列表，赋为TH32CS_SNAPHEAPLIST
        //        要得到指定进程的模块列表，赋为TH32CS_SNAPMODULE
        //        要获取其他标记代表意义，查看MSDN
        // 参数2：指定将要快照的进程ID  如果该参数为0表示快照当前进程
        //         该参数只有在设置了TH32CS_SNAPHEAPLIST或TH32CS_SNAPMOUDLE后才有效，在其他情况下该参数被忽略，所有的进程都会被快照。
        // 返回值：调用成功，返回快照的句柄，调用失败，返回INVAID_HANDLE_VALUE
	if(hShot == NULL) return -1;
	mes.dwSize = sizeof(mes); // 使用PROCESSENTRY32结构时，先设置其大小
	exist = Process32First(hShot,&mes); // 返回快照中第一个进程信息到mes   函数失败返回0
	while(exist)
	{
		// _tcsstr 功能同 strstr
		if(_tcsstr(mes.szExeFile,name) != NULL)
		{
			pid = mes.th32ProcessID;
			break;
		}
		exist = Process32Next(hShot,&mes);
	}
	CloseHandle(hShot);
	return pid;
}
BOOL InjectDll(DWORD dwProcessID, char* dllPath)
{//参数：目标进程ID、DLL路径
	FARPROC FuncAddr = NULL;
	HMODULE hdll = LoadLibrary(TEXT("Kernel32.dll"));//加载DLL
	if (hdll != NULL)
	{
		FuncAddr = GetProcAddress(hdll, "LoadLibraryA");//获取LoadLibraryA函数地址
		if (FuncAddr == NULL) return FALSE;
	}
 
	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION, FALSE, dwProcessID);//获取进程句柄
	if (hProcess == NULL) return FALSE;
	DWORD dwSize = strlen(dllPath) + 1;// dll大小 + 1
	LPVOID RemoteBuf = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);//远程申请内存
	DWORD dwRealSize;
	if (WriteProcessMemory(hProcess, RemoteBuf, dllPath, dwSize, &dwRealSize))//远程写dll进内存
	{
		DWORD dwThreadId;
		HANDLE hRemoteThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)FuncAddr, RemoteBuf, 0, &dwThreadId);//创建远程线程  加载dll
		if (hRemoteThread == NULL)
		{
			VirtualFreeEx(hProcess, RemoteBuf, dwSize, MEM_COMMIT);
			CloseHandle(hProcess);
			return FALSE;
		}
		//释放资源
		WaitForSingleObject(hRemoteThread, INFINITE);
		CloseHandle(hRemoteThread);
		VirtualFreeEx(hProcess, RemoteBuf, dwSize, MEM_COMMIT);
		CloseHandle(hProcess);
		return TRUE;
	}
	else
	{
		VirtualFreeEx(hProcess, RemoteBuf, dwSize, MEM_COMMIT);
		CloseHandle(hProcess);
		return FALSE;
	}
}

int main(int argc, char* argv[])
{
	char* name = "taskmgr.exe";
	char* DllPath = "C:\\VC6.0green\\MyProjects\\Mydll\\Debug\\Mydll.dll";
	DWORD pid = GetpidByName(name);
	if(pid != -1) printf("find %s pid:%d\n",name,pid);
	if(InjectDll(pid,DllPath))
	{
		printf("success\n");
	}
	else
	{
		printf("error\n");
	}
	


	return 0;
}


```

