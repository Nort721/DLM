#include "instutil.h"
#include <shlobj.h>
#include <filesystem>

namespace fs = std::filesystem;

BOOLEAN CopyDlmToSystem() {
    WCHAR appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return FALSE;
    }

    fs::path dlmPath = fs::path(appDataPath) / L"dlm";
    if (!fs::exists(dlmPath)) {
        if (!fs::create_directory(dlmPath)) {
            return FALSE;
        }
    }

    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();

    for (const auto& entry : fs::directory_iterator(exeDir)) {
        if (fs::is_regular_file(entry.path())) {
            try {
                fs::copy(entry.path(), dlmPath / entry.path().filename(), fs::copy_options::overwrite_existing);
            }
            catch (const fs::filesystem_error& e) {
                std::wcerr << L"Error copying file: " << entry.path().filename().c_str() << L" - " << e.what() << std::endl;
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOLEAN AddDlmStartupToRunkey() {
    // Get the AppData path
    WCHAR appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return FALSE;
    }

    // Construct the full path to dlmsrv.exe
    std::wstring dlmExePath = std::wstring(appDataPath) + L"\\dlm\\dlmsrv.exe";

    // Open the Run registry key
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    // Set the value in the registry
    if (RegSetValueExW(hKey, L"DlmService", 0, REG_SZ, reinterpret_cast<const BYTE*>(dlmExePath.c_str()), (dlmExePath.size() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }

    // Close the registry key
    RegCloseKey(hKey);
    return TRUE;
}

BOOLEAN DeleteDlmFromSystem() {
    // ToDo -> unload the driver and stop the service before coninuing to execute

    // Get the AppData path
    WCHAR appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return FALSE;
    }

    // Construct the full path to the dlm folder
    fs::path dlmPath = fs::path(appDataPath) / L"dlm";

    // Check if the dlm folder exists
    if (!fs::exists(dlmPath)) {
        return TRUE; // Folder does not exist, nothing to delete
    }

    // Remove the dlm folder and its contents
    try {
        fs::remove_all(dlmPath);
    }
    catch (const fs::filesystem_error& e) {
        std::wcerr << L"Error deleting folder: " << dlmPath.c_str() << L" - " << e.what() << std::endl;
        return FALSE;
    }

    return TRUE;
}

BOOLEAN RemoveDlmStartupFromRunkey() {
    // Open the Run registry key
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    // Delete the Run key value
    if (RegDeleteValueW(hKey, L"DlmService") != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return FALSE;
    }

    // Close the registry key
    RegCloseKey(hKey);
    return TRUE;
}

BOOLEAN InstallDLM() {
	BOOLEAN status = CopyDlmToSystem();
    if (!status) {
        std::cerr << "Failed to copy DLM files to machine.";
        return FALSE;
    }

	status = AddDlmStartupToRunkey();
    if (!status) {
        std::cerr << "Failed to set the run key value.\n";
        return FALSE;
    }

    return TRUE;
}

BOOLEAN UninstallDLM() {
	BOOLEAN status = DeleteDlmFromSystem();
    if (!status) {
        std::cerr << "Failed remove DLM files from machine.";
        return FALSE;
    }

	status = RemoveDlmStartupFromRunkey();
    if (!status) {
        std::cerr << "Failed to remove the run key value.\n";
        return FALSE;
    }

    return TRUE;
}