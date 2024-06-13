#include "io.h"
#include <setupapi.h>
#include <shlobj.h>
#include <iostream>
#include <string>

#pragma comment(lib, "setupapi.lib")

BOOLEAN LoadKernelModule() {
    // Get the path of the currently running executable
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        std::cerr << "Failed to get executable path." << std::endl;
        return FALSE;
    }

    // Extract the directory from the executable path
    std::string exeDir(exePath);
    size_t pos = exeDir.find_last_of("\\/");
    if (pos == std::string::npos) {
        std::cerr << "Failed to extract directory from executable path." << std::endl;
        return FALSE;
    }
    exeDir = exeDir.substr(0, pos);

    // Path to the INF file
    std::string infPath = exeDir + "\\dlmkrnl.inf";

    // Install the INF file
    if (!SetupCopyOEMInfA(infPath.c_str(), NULL, SPOST_PATH, 0, NULL, 0, NULL, NULL)) {
        std::cerr << "Failed to install INF file: " << infPath << std::endl;
        return FALSE;
    }

    // Command to be executed
    std::string command = exeDir + "\\Loader.exe " + exeDir + "\\gdrv.sys " + exeDir + "\\dlmkrnl.sys";

    // Prepare the command line
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to execute command: " << command << std::endl;
        return FALSE;
    }

    // Wait for the process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "Kernel module loaded successfully." << std::endl;
    return TRUE;
}

int main() {
	std::cout << "- Driver Load Monitor -\n\n";

    if (LoadKernelModule()) {
        std::cout << "Operation completed successfully." << std::endl;
    }
    else {
        std::cout << "Operation failed." << std::endl;
        MessageBoxA(NULL, "Failed to load kernel module.", "DLM Error", MB_OK);
        return 1;
    }

    HANDLE hDriverPort;
    WCHAR PortName[] = L"\\FilterDlmPort";

	while (TRUE) {
        HRESULT Result = FilterConnectCommunicationPort(PortName, 0, NULL, 0, NULL, &hDriverPort);
        if (FAILED(Result)) {
            std::cerr << "[-] failed to connect to driver port." << std::endl;
            return FALSE;
        }

        SendPendingGet(hDriverPort);

        CloseHandle(hDriverPort);
	}

	return 0;
}