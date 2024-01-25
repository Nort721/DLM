#pragma once

#include <winternl.h>

typedef NTSYSCALLAPI NTSTATUS(NTAPI* ntloaddriver)(PUNICODE_STRING DriverServiceName);

#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
#define MAX_PATH_LENGTH 256

void InstallInitHook();