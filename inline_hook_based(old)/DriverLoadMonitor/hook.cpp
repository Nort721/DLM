#pragma warning(disable : 6387)

#include <array>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winternl.h>
#include "hook.h"
#include "errors.h"
#include "fileutil.h"
#include "net.h"

ntloaddriver NtLoadDriverOrigAddr;
LPVOID orig_bytes;

DWORD ChangeMemoryPermissions(
    _In_ void* const address, 
    _In_ const size_t size, 
    _In_ const DWORD protections
    )
{
    DWORD oldProtections{};
    BOOL result = VirtualProtect(address, size, protections, &oldProtections);
    if (!result)
    {
        ERROR_WITH_CODE("VirtualProtect failed.");
    }
    return oldProtections;
}

void RewriteOriginalBytes(void* const targetAddress)
{
    const auto oldProtections = ChangeMemoryPermissions(targetAddress, 25, PAGE_EXECUTE_READWRITE);
    memcpy_s(targetAddress, 25, orig_bytes, 25);
}

std::array<unsigned char, 12> CreateInlineHookBytes(const void* const destinationAddress)
{
    std::array<unsigned char, 12> jumpBytes{ {
            0x48, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
                0xFF, 0xE0
        } };

    size_t address = reinterpret_cast<size_t>(destinationAddress);
    std::memcpy(&jumpBytes[2], &address, sizeof(void*));

    return jumpBytes;
}

void* SaveBytes(
    _In_ void* const targetAddress, 
    _In_ const size_t size
    )
{
    LPVOID originalBytes = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!originalBytes)
    {
        ERROR_WITH_CODE("VirtualAlloc failed.");
    }

    std::memcpy(originalBytes, targetAddress, size);

    return originalBytes;
}

void InstallInlineHook(
    _In_ void* const targetAddress, 
    _In_ const void* hookAddress
    )
{
    orig_bytes = SaveBytes(targetAddress, 25);

    std::array<unsigned char, 12> hookBytes = CreateInlineHookBytes(hookAddress);

    DWORD oldProtections = ChangeMemoryPermissions(targetAddress, hookBytes.size(), PAGE_EXECUTE_READWRITE);

    std::memcpy(targetAddress, hookBytes.data(), hookBytes.size());

    ChangeMemoryPermissions(targetAddress, hookBytes.size(), oldProtections);

    FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
}

NTSTATUS NTAPI HookNtLoadDriver(PUNICODE_STRING DriverServiceName) {
    // cut name from reg path
    PWSTR driverServiceName = GetBaseNameFromFullNameWide(DriverServiceName->Buffer);

    WCHAR* servicePath = GetServiceBinaryName(driverServiceName);

    // approve if binary path is null
    BOOL approved = TRUE;

    if (servicePath != NULL)
    {
        // removing the dumb /??/ prefix from start of the string
        RemovePrefix(servicePath);
        approved = VerifyDriverBinary(servicePath);
    }

    NTSTATUS status = STATUS_ACCESS_DENIED;

    if (approved)
    {
        printf("DLM - APPROVED.\n");
        RewriteOriginalBytes(NtLoadDriverOrigAddr);
        status = NtLoadDriverOrigAddr(DriverServiceName);
        InstallInlineHook(NtLoadDriverOrigAddr, &HookNtLoadDriver);
    }
    else
    {
        printf("DLM - REJECTED.\n");
    }

    return status;
}

void InstallInitHook() {
    NtLoadDriverOrigAddr = (ntloaddriver)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtLoadDriver");
    InstallInlineHook(NtLoadDriverOrigAddr, &HookNtLoadDriver);
}

