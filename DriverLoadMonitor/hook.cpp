#include <array>
#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <winternl.h>
#include <malloc.h>
#include "hook.h"

typedef NTSYSCALLAPI NTSTATUS (NTAPI* ntloaddriver)(PUNICODE_STRING DriverServiceName);

#define MAX_PATH_LENGTH 256

ntloaddriver NtLoadDriverOrigAddr;
LPVOID orig_byes;

DWORD ChangeMemoryPermissions(void* const address, const size_t size, const DWORD protections) {
    DWORD oldProtections{};
    BOOL result = VirtualProtect(address, size, protections, &oldProtections);
    if (!result) {
        std::cout << "error in VirtualProtect" << std::endl;
    }
    return oldProtections;
}

void RewriteOriginalBytes(void* const targetAddress, LPVOID orig_bytes) {
    const auto oldProtections = ChangeMemoryPermissions(targetAddress, 25, PAGE_EXECUTE_READWRITE);
    memcpy_s(targetAddress, 25, orig_byes, 25);
}

std::array<unsigned char, 12> CreateInlineHookBytes(const void* const destinationAddress) {
    std::array<unsigned char, 12> jumpBytes{ {
            0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
                0xFF, 0xE0
        } };

    size_t address = reinterpret_cast<size_t>(destinationAddress);
    std::memcpy(&jumpBytes[2], &address, sizeof(void*));

    return jumpBytes;
}

void* SaveBytes(void* const targetAddress, const size_t size) {
    LPVOID originalBytes = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!originalBytes)
    {
        std::cout << "Failed in VirtualAlloc" << std::endl;
    }

    std::memcpy(originalBytes, targetAddress, size);

    return originalBytes;
}

void InstallInlineHook(void* const targetAddress, const void* hookAddress) {
    orig_byes = SaveBytes(targetAddress, 25);

    std::array<unsigned char, 12> hookBytes = CreateInlineHookBytes(hookAddress);

    DWORD oldProtections = ChangeMemoryPermissions(targetAddress, hookBytes.size(), PAGE_EXECUTE_READWRITE);

    std::memcpy(targetAddress, hookBytes.data(), hookBytes.size());

    ChangeMemoryPermissions(targetAddress, hookBytes.size(), oldProtections);

    FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
}

WCHAR* GetServiceBinaryName(const WCHAR* ServiceName) {
    SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        wprintf(L"OpenSCManager failed (%d)\n", GetLastError());
        return NULL;
    }

    SC_HANDLE hService = OpenServiceW(hSCManager, ServiceName, SERVICE_QUERY_CONFIG);
    if (!hService) {
        wprintf(L"OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return NULL;
    }

    WCHAR* servicePath = NULL;
    DWORD dwBytesNeeded = sizeof(QUERY_SERVICE_CONFIGW);
    LPQUERY_SERVICE_CONFIGW pConfig;

    do {
        servicePath = (WCHAR*)malloc(dwBytesNeeded);
        if (!servicePath) {
            wprintf(L"Memory allocation failed\n");
            break;
        }

        pConfig = (LPQUERY_SERVICE_CONFIGW)servicePath;

        if (QueryServiceConfigW(hService, pConfig, dwBytesNeeded, &dwBytesNeeded)) {
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return _wcsdup(pConfig->lpBinaryPathName);
        }
        else {
            free(servicePath);
            servicePath = NULL;
        }
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return NULL;
}

WCHAR* GetServiceBinaryName(PUNICODE_STRING ServiceName) {
    SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        wprintf(L"OpnSm (%d)\n", GetLastError());
        return NULL;
    }

    WCHAR serviceNameBuffer[MAX_PATH_LENGTH];
    if (wcsncpy_s(serviceNameBuffer, MAX_PATH_LENGTH, ServiceName->Buffer, ServiceName->Length / sizeof(WCHAR)) != 0) {
        wprintf(L"Service name conversion failed\n");
        CloseServiceHandle(hSCManager);
        return NULL;
    }

    SC_HANDLE hService = OpenServiceW(hSCManager, serviceNameBuffer, SERVICE_QUERY_CONFIG);
    if (!hService) {
        wprintf(L"OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return NULL;
    }

    WCHAR* servicePath = NULL;
    DWORD dwBytesNeeded = sizeof(QUERY_SERVICE_CONFIGW);
    LPQUERY_SERVICE_CONFIGW pConfig;

    do {
        servicePath = (WCHAR*)malloc(dwBytesNeeded);
        if (!servicePath) {
            wprintf(L"Memory allocation failed\n");
            break;
        }

        pConfig = (LPQUERY_SERVICE_CONFIGW)servicePath;

        if (QueryServiceConfigW(hService, pConfig, dwBytesNeeded, &dwBytesNeeded)) {
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return _wcsdup(pConfig->lpBinaryPathName);
        }
        else {
            free(servicePath);
            servicePath = NULL;
        }
    } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return NULL;
}

PWSTR GetBaseNameFromFullNameWide(PWSTR fullName) {
    SIZE_T fullnameLength = wcslen(fullName);

    for (SIZE_T i = fullnameLength; i > 0; i--) {
        if (*(fullName + i) == L'\\') {
            return fullName + i + 1;
        }
    }

    return fullName;
}

void RemovePrefix(WCHAR* path) {
    // Check if the path starts with "\??\"
    if (wcsncmp(path, L"\\??\\", wcslen(L"\\??\\")) == 0) {
        // Move the pointer past the prefix
        wcsncpy(path, path + wcslen(L"\\??\\"), wcslen(path) - wcslen(L"\\??\\") + 1);
    }
}

NTSTATUS NTAPI HookNtLoadDriver(PUNICODE_STRING DriverServiceName) {
    RewriteOriginalBytes(NtLoadDriverOrigAddr, orig_byes);

    // parameter contains registry path, here we cut the name of the service from path
    PWSTR driverServiceName = GetBaseNameFromFullNameWide(DriverServiceName->Buffer);
    printf("service name: %ws\n", driverServiceName);

    // get the path of the binary of the service
    WCHAR* servicePath = GetServiceBinaryName(driverServiceName);

    if (servicePath != NULL)
    {
        // removing the dumb /??/ prefix from start of the string
        RemovePrefix(servicePath);
        printf("service binary %ws\n", servicePath);
    }

    NTSTATUS status = NtLoadDriverOrigAddr(DriverServiceName);

    InstallInlineHook(NtLoadDriverOrigAddr, &HookNtLoadDriver);

    return status;
}

FARPROC GetFuncAddr(LPCSTR dll, LPCSTR func_name, bool is_module_loaded)
{
    HMODULE hmod;
    if (is_module_loaded)
    {
        hmod = GetModuleHandleA(dll);
    }
    else
    {
        hmod = LoadLibraryA(dll);
    }
    return GetProcAddress(hmod, func_name);
}

void init() {
    NtLoadDriverOrigAddr = (ntloaddriver)GetFuncAddr("ntdll.dll", "NtLoadDriver", true);
    InstallInlineHook(NtLoadDriverOrigAddr, &HookNtLoadDriver);
}

