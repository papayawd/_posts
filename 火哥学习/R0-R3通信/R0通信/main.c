#include<ntddk.h>

#define DEVICE_NAME L"\\device\\PAPAYADEVICE"
#define SYMBOL_NAME_LINK L"\\??\\papayadevice"

#define CODE_READ CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CODE_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

VOID DriverUnload(PDRIVER_OBJECT pDriver) {
	UNICODE_STRING symbolNameLink;
	RtlInitUnicodeString(&symbolNameLink, SYMBOL_NAME_LINK);
	IoDeleteSymbolicLink(&symbolNameLink);
	IoDeleteDevice(pDriver->DeviceObject);

	DbgPrint("driver unload\n");
}
NTSTATUS DeviceCreate(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp) {
	DbgPrint("device link\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS DeviceClose(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp) {
	DbgPrint("devide close\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS DeviceDispatch(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp) {
	PIO_STACK_LOCATION psl = IoGetCurrentIrpStackLocation(Irp);
	ULONG code = psl->Parameters.DeviceIoControl.IoControlCode;
	PVOID systemBuf = Irp->AssociatedIrp.SystemBuffer;
	ULONG inLen = psl->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLen = psl->Parameters.DeviceIoControl.OutputBufferLength;

	switch (code) {
	case CODE_READ:
		DbgPrint("CODE_READ %X\n", CODE_READ);
		memcpy(systemBuf, "12345678", sizeof("12345678"));
		Irp->IoStatus.Information = sizeof("12345678");
		break;
	case CODE_WRITE:
		DbgPrint("CODE_WRITE %X\n", CODE_WRITE);
		DbgPrint("write %s\n", (char*)systemBuf);
		Irp->IoStatus.Information = 0;
		break;
	}
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg) {

	UNICODE_STRING deviceName;
	UNICODE_STRING symbolNameLink;
	PDRIVER_OBJECT pdriverobj;
	NTSTATUS status;
	pDriver->DriverUnload = DriverUnload;
	DbgPrint("driver load\n");
	RtlInitUnicodeString(&deviceName, DEVICE_NAME);
	RtlInitUnicodeString(&symbolNameLink, SYMBOL_NAME_LINK);
	status = IoCreateDevice(pDriver, 0, &deviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &pdriverobj);
	if (!NT_SUCCESS(status)) {
		DbgPrint("create device error\n");
		return status;
	}

	status = IoCreateSymbolicLink(&symbolNameLink, &deviceName);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(pdriverobj);
		DbgPrint("create device link error\n");
		return status;
	}

	pDriver->Flags |= DO_BUFFERED_IO;
	pDriver->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceDispatch;
	

	return STATUS_SUCCESS;
}