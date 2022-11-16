#include <ntddk.h>

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union {
        struct {
            ULONG TimeDateStamp;
        };
        struct {
            PVOID LoadedImports;
        };
    };
    struct _ACTIVATION_CONTEXT* EntryPointActivationContext;

    PVOID PatchInformation;

} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

HANDLE hThread;

struct PART_OF_DRIVER_OBJECT {
    ULONG DriverSize;
    PVOID DriverSection;
    PVOID DriverStart;
    PDRIVER_INITIALIZE DriverInit;
    PFAST_IO_DISPATCH FastIoDispatch;
    PDRIVER_EXTENSION DriverExtension;
    PDRIVER_STARTIO DriverStartIo;
}tmp_data;

// 修复特征码 ，但是需要在驱动停止之前恢复，否则无法暂停并且一直存在。。。。
VOID RecoverData(PDRIVER_OBJECT pDriver) {
    pDriver->DriverSize = tmp_data.DriverSize;
    pDriver->DriverSection = tmp_data.DriverSection;
    pDriver->DriverStart = tmp_data.DriverStart;
    pDriver->DriverInit = tmp_data.DriverInit;
    pDriver->FastIoDispatch = tmp_data.FastIoDispatch;
    pDriver->DriverExtension = tmp_data.DriverExtension;
    pDriver->DriverStartIo = tmp_data.DriverStartIo;
}

VOID DriverUnload(PDRIVER_OBJECT pDriver) {
    
    DbgPrint("卸载了\n");
}
VOID ThreadRun(_In_ PVOID StartContext) {
    DbgPrint("开始执行111\n");
    LARGE_INTEGER time;
    time.QuadPart = -30 * 1000 * 1000;
    KeDelayExecutionThread(KernelMode, FALSE, &time);
    PDRIVER_OBJECT pDriver = (PDRIVER_OBJECT)StartContext;
    // 保存可能被检测出的特征码   
    tmp_data.DriverSize = pDriver->DriverSize;
    tmp_data.DriverSection = pDriver->DriverSection;
    tmp_data.DriverStart = pDriver->DriverStart;
    tmp_data.DriverInit = pDriver->DriverInit;
    tmp_data.FastIoDispatch = pDriver->FastIoDispatch;
    tmp_data.DriverExtension = pDriver->DriverExtension;
    tmp_data.DriverStartIo = pDriver->DriverStartIo;

    // 抹去可能被检测出的特征码   
    pDriver->DriverSize = 0;
    pDriver->DriverSection = NULL;
    pDriver->DriverStart = NULL;
    pDriver->DriverInit = NULL;
    pDriver->FastIoDispatch = NULL;
    pDriver->DriverExtension = NULL;
    pDriver->DriverStartIo = NULL;
    ZwClose(hThread);
    DbgPrint("执行结束111\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg) {

	DbgPrint("加载了\n");
	pDriver->DriverUnload = DriverUnload;
	
    PLDR_DATA_TABLE_ENTRY pList = (PLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
    PLDR_DATA_TABLE_ENTRY pCur = pList;
    PLDR_DATA_TABLE_ENTRY pFlink = pList->InLoadOrderLinks.Flink;
    PLDR_DATA_TABLE_ENTRY pBlink = pList->InLoadOrderLinks.Blink;
    pFlink->InLoadOrderLinks.Blink = pBlink;
    pBlink->InLoadOrderLinks.Flink = pFlink;

    PsCreateSystemThread(&hThread, GENERIC_ALL, NULL, NULL, NULL, ThreadRun, pDriver);

    return STATUS_SUCCESS;
}