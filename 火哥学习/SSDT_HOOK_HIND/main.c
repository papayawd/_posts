
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
	KSERVICE_TABLE_DESCRIPTOR serviceTableShadow;
	KSERVICE_TABLE_DESCRIPTOR u3; // δʹ��
	KSERVICE_TABLE_DESCRIPTOR u4; // δʹ��

} ServiceDescriptorTable, * PServiceDescriptorTable;

PServiceDescriptorTable MYSSDT;

extern PServiceDescriptorTable KeServiceDescriptorTable; // ����KeServiceDescriptorTable

struct _HookItem {
	ULONG oldAddress;
	ULONG newAddress;
	ULONG code;
	ULONG hookSuccess;
}HookItem[HOOK_ITEM_NUM];

HANDLE hThread;

ULONG isRunning;

__declspec(naked) void wp_off() // �ر�д����
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

__declspec(naked) void wp_on() // ����д����
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

	DbgPrint("NtReadVirtualMemory ���� HOOK\n");

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

VOID SetHook(int code, ULONG hookAddress) {
	int i, index = -1;
	for (i = 0; i < HOOK_ITEM_NUM; i++) {
		if (!HookItem[i].hookSuccess) {
			index = i;
			break;
		}
	}
	if (index == -1) {
		DbgPrint("Hook������Hook %xʧ��\n", code);
		return;
	}
	HookItem[index].oldAddress = *(((PULONG)(MYSSDT->serviceTable.Base)) + code);
	HookItem[index].newAddress = hookAddress;
	HookItem[index].code = code;
	HookItem[index].hookSuccess = 1;

	wp_off();
	*(((PULONG*)(MYSSDT->serviceTable.Base)) + code) = hookAddress;
	wp_on();

	DbgPrint("�ɹ�Hook %x\n", code);
}
VOID UnHook(int code) {
	int i;
	for (i = 0; i < HOOK_ITEM_NUM; i++) {
		if (HookItem[i].hookSuccess && HookItem[i].code == code) {
			DbgPrint("UnHook %x\n", code);

			wp_off();
			*(((PULONG*)(MYSSDT->serviceTable.Base)) + code) = HookItem[i].oldAddress;
			wp_on();

			return;
		}
	}
	DbgPrint("�������� %x ��HOOK\n", code);
}

/*
* ֻ��ĳ�������ڵ������̵߳�  �ᱻPChunter����� ����ѧϰһ��˼·
* 
* 
	KRPC�����ҵ���ǰ�߳�--�ҵ���ǰ����--�ҵ���ǰ���������--�����ҵ�Ŀ�����

	����--�����µ��׸��߳�--�߳�����  �߳�E0λ�þ���SSDT���ַ

	XP�£�����KeServiceDescriptorTable��������KeServiceDescriptorTableShadow һ��Table���ĸ�ItemҲ����0x40��С
	KeServiceDescriptorTable = 0x805634e0
	KeServiceDescriptorTableShadow = 0x80563520

	����ֻHOOK ServiceDescriptorTable   ��˼��ֻ�滻��GUI�̵߳�SSDT��
	ҪHOOK ServiceDescriptorTableShadow�Ļ� �編���Ƹ���һ��Ȼ���޸ģ�һֱ�滻���ɡ�

*/

NTSTATUS InitMYSSDT() {
	MYSSDT = ExAllocatePoolWithTag(NonPagedPool, sizeof(ServiceDescriptorTable), 'abc');
	if (MYSSDT == NULL) {
		DbgPrint("����MYSSDTʧ�ܣܣ�");
		return STATUS_UNSUCCESSFUL;
	}
	memset(MYSSDT, 0, sizeof(ServiceDescriptorTable));
	MYSSDT->serviceTable.Base = ExAllocatePoolWithTag(NonPagedPool,
		KeServiceDescriptorTable->serviceTable.Limit * 4, 'def');
	if (MYSSDT->serviceTable.Base == NULL) {
		DbgPrint("����MYSSDTʧ�ܣܣ�");
		ExFreePoolWithTag(MYSSDT, 'abc');
		return STATUS_UNSUCCESSFUL;
	}

	// ���� serviceTable
	memcpy(MYSSDT->serviceTable.Base, KeServiceDescriptorTable->serviceTable.Base,
		KeServiceDescriptorTable->serviceTable.Limit * 4);
	MYSSDT->serviceTable.Count = KeServiceDescriptorTable->serviceTable.Count;
	MYSSDT->serviceTable.Limit = KeServiceDescriptorTable->serviceTable.Limit;
	MYSSDT->serviceTable.Number = KeServiceDescriptorTable->serviceTable.Number;

	// ֱ����ԭ���� serviceTableShadow
	MYSSDT->serviceTableShadow.Base = KeServiceDescriptorTable->serviceTableShadow.Base;
	MYSSDT->serviceTableShadow.Limit = KeServiceDescriptorTable->serviceTableShadow.Limit;
	MYSSDT->serviceTableShadow.Count = KeServiceDescriptorTable->serviceTableShadow.Count;
	MYSSDT->serviceTableShadow.Number = KeServiceDescriptorTable->serviceTableShadow.Number;

	return STATUS_SUCCESS;
}

VOID DestoryMYSSDT() {
	if (MYSSDT != NULL) {
		if (MYSSDT->serviceTable.Base != NULL) {
			ExFreePoolWithTag(MYSSDT->serviceTable.Base, 'def');
			MYSSDT->serviceTable.Base = NULL;
		}
		ExFreePoolWithTag(MYSSDT, 'abc');
	}
	MYSSDT = NULL;
}

ULONG FindProcess(char* pName) {
	ULONG curProcess;
	__asm
	{
		mov eax, fs: [0x124]
		mov eax, [eax + 0x44]
		mov curProcess, eax
	}
	PLIST_ENTRY activeProcessList = (PLIST_ENTRY)(curProcess + 0x88);
	do {
		ULONG process = (ULONG)activeProcessList - 0x88;
		if (!strcmp((char*)process + 0x174, pName)) {
			return process;
		}
		activeProcessList = activeProcessList->Flink;
	} while ((ULONG)activeProcessList - 0x88 != curProcess);
	return 0;
}

VOID ReplacethreadsE0(ULONG process) {
	ULONG curList = process + 0x190;
	PLIST_ENTRY threadList = (PLIST_ENTRY)(process + 0x190);
	do {
		ULONG thread = (ULONG)threadList - 0x22c;
		if (thread < 0x80000000) continue;
		if (*(PULONG)(thread + 0xE0) == (PULONG)KeServiceDescriptorTable) { // �������֮ǰ��SSDT
			wp_off();
			*(PULONG)(thread + 0xE0) = (PULONG)MYSSDT;
			wp_on();
		}
		threadList = threadList->Flink;
	} while ((ULONG)threadList != curList);
}

VOID ThreadRun() {
	isRunning = 1;
	LARGE_INTEGER timer = { 0 };
	timer.QuadPart = -10 * 1000 * 1000;
	while (isRunning) {
		KeDelayExecutionThread(KernelMode, FALSE, &timer);
		ULONG process = FindProcess("runing.exe");
		DbgPrint("FindProcess=%x\n", process);
		if(process){
			ReplacethreadsE0(process);
		}

	}
}

VOID DriverUnload(PDRIVER_OBJECT pDriver) {
	UnHook(0xBA);
	isRunning = 0;
	LARGE_INTEGER timer = { 0 };
	timer.QuadPart = -30 * 1000 * 1000;
	KeDelayExecutionThread(KernelMode, TRUE, &timer);
	DestoryMYSSDT();
	DbgPrint("ж����\n");
}



NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg) {

	DbgPrint("������\n");
	pDriver->DriverUnload = DriverUnload;
	
	DbgPrint("KeServiceDescriptorTable=%x\n", (ULONG)KeServiceDescriptorTable);
	if (!NT_SUCCESS(InitMYSSDT())) {
		DestoryMYSSDT();
		return STATUS_UNSUCCESSFUL;
	}

	DbgPrint("SSDT ��ʼ���ɹ�\n");

	SetHook(0xBA, (ULONG)MyNtReadVirtualMemory);

	PsCreateSystemThread(&hThread, 0, NULL, NULL, NULL, ThreadRun, NULL);

	DbgPrint("MYSSDT=%x\n", (ULONG)MYSSDT);
	
	return STATUS_SUCCESS;
}
