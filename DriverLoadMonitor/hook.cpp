#include <winsock2.h>
#include <array>
#include <iostream>
#include <Windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <winternl.h>
#include <malloc.h>
#include "hook.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

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

ULONG CalcMemHash(const PUCHAR data, size_t size) {
    ULONG hash = 5381;

    for (size_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + data[1];
    }

    return hash;
}

BOOL ReadCalcSend(WCHAR* binaryPath)
{
    HANDLE hFile = NULL;
    DWORD dwLength = NULL;
    DWORD dwBytesRead = 0;
    LPVOID lpBuffer = NULL;

    hFile = CreateFileW(binaryPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        perror("Failed to open the DLL file.\n");
        return 1;
    }
    dwLength = GetFileSize(hFile, NULL);
    if (dwLength == INVALID_FILE_SIZE || dwLength == 0) {
        perror("Failed to get the DLL file size\n");
        CloseHandle(hFile);
        return 1;
    }
    lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
    if (!lpBuffer) {
        perror("Failed to allocate a buffer!\n");
        CloseHandle(hFile);
        return 1;
    }
    if (ReadFile(hFile, lpBuffer, dwLength, &dwBytesRead, NULL) == FALSE) {
        perror("Failed to read dll raw data\n");
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        return 1;
    }

    ULONG hash = CalcMemHash((PUCHAR)lpBuffer, dwLength);

    // Convert ULONG hash to string
    char str[20]; // Adjust the size based on the expected length of the ULONG hash
    sprintf(str, "%ld\n", hash);

    //printf("hash: %s", str);

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        WSACleanup();
        return 1;
    }

    // Create a socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        printf("Failed to create socket\n");
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        WSACleanup();
        return 1;
    }

    // Specify the server address and port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        printf("Failed to connect to the server\n");
        closesocket(clientSocket);
        CloseHandle(hFile);
        HeapFree(GetProcessHeap(), 0, lpBuffer);
        WSACleanup();
        return 1;
    }

    // Send the hash as a string
    if (send(clientSocket, str, sizeof(str), 0) == SOCKET_ERROR) {
        printf("Failed to send hash to the server\n");
    }

    // Receive the response from the server
    char response[256];
    int bytesRead = recv(clientSocket, response, sizeof(response) - 1, 0);
    if (bytesRead > 0) {
        response[bytesRead] = '\0'; // Null-terminate the received data
        //printf("Server response: %s\n", response);
    }

    // Cleanup
    closesocket(clientSocket);
    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, lpBuffer);
    WSACleanup();

    if (strstr(response, "approved") != NULL)
    {
        return TRUE;
    }
    return FALSE;
}

NTSTATUS NTAPI HookNtLoadDriver(PUNICODE_STRING DriverServiceName) {
    RewriteOriginalBytes(NtLoadDriverOrigAddr, orig_byes);

    // parameter contains registry path, here we cut the name of the service from path
    PWSTR driverServiceName = GetBaseNameFromFullNameWide(DriverServiceName->Buffer);

    // get the path of the binary of the service
    WCHAR* servicePath = GetServiceBinaryName(driverServiceName);

    if (servicePath != NULL)
    {
        // removing the dumb /??/ prefix from start of the string
        RemovePrefix(servicePath);
        //printf("service binary %ws\n", servicePath);
    }

    BOOL approved = ReadCalcSend(servicePath);
    NTSTATUS status;

    if (approved)
    {
        printf("Driver loading monitored - APPROVED.\n");
        status = NtLoadDriverOrigAddr(DriverServiceName);
    }
    else
    {
        printf("Driver loading monitored - REJECTED.\n");
    }

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

