#pragma once

#include <Windows.h>

WCHAR* GetServiceBinaryName(const WCHAR* ServiceName);
PWSTR GetBaseNameFromFullNameWide(PWSTR fullName);
void RemovePrefix(WCHAR* path);
ULONG CalcMemHash(
    _In_ const PUCHAR data,
    _In_ size_t size
);
BOOL ReadBinary(
    _In_ WCHAR* binaryPath,
    _Out_ LPVOID* lpBuffer,
    _In_ DWORD* dwLength
);