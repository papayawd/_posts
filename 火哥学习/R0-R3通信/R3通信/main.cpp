#include <Windows.h>
#include <WinIoCtl.h>
#include <stdio.h>
#include <stdlib.h>


#define SYMBOL_NAME_LINK L"\\\\.\\papayadevice"

#define CODE_READ CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CODE_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

BOOLEAN OpenDevice(HANDLE* handle) {
	HANDLE _handle = CreateFile(SYMBOL_NAME_LINK, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*handle = _handle;
	return (int)_handle > 0;
}

void CloseDevice(HANDLE handle) {
	CloseHandle(handle);
}
BOOLEAN SendCode(HANDLE handle, DWORD code, PVOID inData, ULONG inLen, PVOID outData, ULONG outLen, LPDWORD retLen) {
	return DeviceIoControl(handle, code, inData, inLen, outData, outLen, retLen,NULL);
}

int main() {

	HANDLE handle;
	if (!OpenDevice(&handle)) {
		printf("open device error\n");
		return 0;
	}

	char buf[30] = { 0 };
	DWORD len = 0;
	SendCode(handle, CODE_READ, buf, 30, buf, 30, &len);
	CloseDevice(handle);
	printf("buf=%s\n", buf);
	system("pause");
	return 0;
}