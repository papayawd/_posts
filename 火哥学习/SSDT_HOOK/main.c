
#include <ntddk.h>

#define HOOK_ITEM_NUM 50

NTSTATUS __stdcall MyNtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, 
	PVOID Buffer, SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead);

typedef struct _KSERVICE_TABLE_DESCRIPTOR {
	PULONG Base;
	PULONG Count;
	ULONG Limit;
	PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, * PKSERVICE_TABLE_DESCRIPTOR;


typedef struct _ServiceDescriptorTable {
	KSERVICE_TABLE_DESCRIPTOR serviceTable;
	KSERVICE_TABLE_DESCRIPTOR serviceTable2;

} ServiceDescriptorTable, * PServiceDescriptorTable;

extern PServiceDescriptorTable KeServiceDescriptorTable; // 导入KeServiceDescriptorTable

struct _HookItem {
	ULONG oldAddress;
	ULONG newAddress;
	ULONG code;
	ULONG hookSuccess;
}HookItem[HOOK_ITEM_NUM];

__declspec(naked) void wp_off() // 关闭写保护
{
	__asm
	{

		cli;
		mov eax, cr0;
		and eax, 0xfffeffff;
		mov cr0, eax;
		ret;
	}
}

__declspec(naked) void wp_on() // 开启写保护
{
	__asm
	{
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		sti;
		ret;
	}
}

ULONG FindAddress(ULONG newAddress) {
	int i;
	for (i = 0; i < HOOK_ITEM_NUM; i++) {
		if (HookItem[i].hookSuccess && newAddress == HookItem[i].newAddress) {
			return HookItem[i].oldAddress;
		}
	}
	return 0;
}

NTSTATUS __stdcall MyNtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, 
	PVOID Buffer, SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead) {

	DbgPrint("NtReadVirtualMemory 被我 HOOK\n");

	NTSTATUS result = STATUS_SUCCESS;
	ULONG oldAddress = FindAddress(MyNtReadVirtualMemory);
	if (oldAddress) {
		__asm 
		{
			push NumberOfBytesRead
			push NumberOfBytesToRead
			push Buffer
			push BaseAddress
			push ProcessHandle

			call oldAddress;

			mov result, eax;

		}
	}
	return result;
}

VOID SetHook(int code,ULONG hookAddress) {
	int i,index = -1;
	for (i = 0; i < HOOK_ITEM_NUM; i++) {
		if (!HookItem[i].hookSuccess) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		DbgPrint("Hook已满，Hook %x失败\n", code);
		return;
	}
	HookItem[index].oldAddress = *(((PULONG)(KeServiceDescriptorTable->serviceTable.Base)) + code);
	HookItem[index].newAddress = hookAddress;
	HookItem[index].code = code;
	HookItem[index].hookSuccess = 1;

	wp_off();
	*(((PULONG*)(KeServiceDescriptorTable->serviceTable.Base)) + code) = hookAddress;
	wp_on();

	DbgPrint("成功Hook %x\n", code);
}
VOID UnHook(int code) {
	int i ;
	for (i = 0; i < HOOK_ITEM_NUM; i++) {
		if (HookItem[i].hookSuccess && HookItem[i].code == code) {
			DbgPrint("UnHook %x\n", code);

			wp_off();
			*(((PULONG*)(KeServiceDescriptorTable->serviceTable.Base)) + code) = HookItem[i].oldAddress;
			wp_on();

			return;
		}
	}
	DbgPrint("并不存在 %x 的HOOK\n", code);
}

VOID DriverUnload(PDRIVER_OBJECT pDriver) {
	UnHook(0xBA);
	DbgPrint("卸载了\n");
}



NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg) {

	DbgPrint("加载了\n");
	pDriver->DriverUnload = DriverUnload;

	SetHook(0xBA,(ULONG)MyNtReadVirtualMemory);

	return STATUS_SUCCESS;
}