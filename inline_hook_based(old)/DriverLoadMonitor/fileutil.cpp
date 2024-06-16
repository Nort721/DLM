#pragma warning(disable : 6101)

#include "fileutil.h"

#include <stdio.h>
#include "errors.h"

WCHAR* GetServiceBinaryName(const WCHAR* ServiceName)
{
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

PWSTR GetBaseNameFromFullNameWide(PWSTR fullName)
{
    SIZE_T fullnameLength = wcslen(fullName);

    for (SIZE_T i = fullnameLength; i > 0; i--) {
        if (*(fullName + i) == L'\\') {
            return fullName + i + 1;
        }
    }

    return fullName;
}

void RemovePrefix(WCHAR* path)
{
    // Check if the path starts with "\??\"
    if (wcsncmp(path, L"\\??\\", wcslen(L"\\??\\")) == 0) {
        // Move the pointer past the prefix
        wcsncpy(path, path + wcslen(L"\\??\\"), wcslen(path) - wcslen(L"\\??\\") + 1);
    }
}

ULONG CalcMemHash(
    _In_ const PUCHAR data, 
    _In_ size_t size
    )
{
    ULONG hash = 5381;

    for (size_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + data[i];
    }

    return hash;
}

BOOL ReadBinary(
    _In_ WCHAR* binaryPath, 
    _Out_ LPVOID* lpBuffer, 
    _In_ DWORD* dwLength
    )
{
    HANDLE hFile = CreateFileW(binaryPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        ERROR_WITH_CODE("Failed to open the file.");
        return FALSE;
    }

    *dwLength = GetFileSize(hFile, NULL);
    if (*dwLength == INVALID_FILE_SIZE || dwLength == 0) {
        ERROR_WITH_CODE("Failed to get the file size.");
        CloseHandle(hFile);
        return FALSE;
    }

    *lpBuffer = HeapAlloc(GetProcessHeap(), 0, *dwLength);
    if (!*lpBuffer) {
        ERROR_WITH_CODE("Failed to allocate a buffer.");
        CloseHandle(hFile);
        return FALSE;
    }

    if (ReadFile(hFile, *lpBuffer, *dwLength, NULL, NULL) == FALSE) {
        ERROR_WITH_CODE("Failed to read file raw data.");
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(), 0, *lpBuffer);
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}