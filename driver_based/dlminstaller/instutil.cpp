#include "instutil.h"
#include <shlobj.h>
#include <filesystem>
#include <taskschd.h>
#include <comdef.h>
#include <atlbase.h>
#include <shlobj.h>
#include <iostream>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

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

BOOLEAN ScheduleStartupTask() {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "CoInitializeEx failed: " << _com_error(hr).ErrorMessage() << std::endl;
        return FALSE;
    }

    // Create Task Service instance
    CComPtr<ITaskService> pService;
    hr = pService.CoCreateInstance(CLSID_TaskScheduler);
    if (FAILED(hr)) {
        std::cerr << "Failed to create TaskService instance: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Connect to the task service
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::cerr << "ITaskService::Connect failed: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Get the root task folder
    CComPtr<ITaskFolder> pRootFolder;
    hr = pService->GetFolder(_bstr_t("\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::cerr << "Cannot get root folder pointer: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Define the task name
    LPCWSTR wszTaskName = L"DlmServiceStartupTask";

    // If the task already exists, delete it
    pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

    // Create the task definition
    CComPtr<ITaskDefinition> pTask;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr)) {
        std::cerr << "Failed to create task definition: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Get the registration info for setting the task's description
    CComPtr<IRegistrationInfo> pRegInfo;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr)) {
        std::cerr << "Cannot get registration info pointer: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }
    pRegInfo->put_Author(_bstr_t("AuthorName"));
    pRegInfo->put_Description(_bstr_t("Starts dlmsrv.exe at Windows startup."));

    // Create the principal for setting the task to run with highest privileges
    CComPtr<IPrincipal> pPrincipal;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr)) {
        std::cerr << "Cannot get principal pointer: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }
    pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

    // Create the trigger to launch the task at startup
    CComPtr<ITriggerCollection> pTriggerCollection;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr)) {
        std::cerr << "Cannot get trigger collection: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    CComPtr<ITrigger> pTrigger;
    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    if (FAILED(hr)) {
        std::cerr << "Cannot create logon trigger: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Create the action to run the program
    CComPtr<IActionCollection> pActionCollection;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr)) {
        std::cerr << "Cannot get action collection pointer: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    CComPtr<IAction> pAction;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    if (FAILED(hr)) {
        std::cerr << "Cannot create action: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    CComPtr<IExecAction> pExecAction;
    hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
    if (FAILED(hr)) {
        std::cerr << "QueryInterface call failed for IExecAction: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Get the path to the AppData folder
    WCHAR appDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::cerr << "SHGetFolderPathW failed" << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Construct the full path to dlmsrv.exe
    std::wstring dlmExePath = std::wstring(appDataPath) + L"\\dlm\\dlmsrv.exe";

    // Set the path of the executable to the action
    hr = pExecAction->put_Path(_bstr_t(dlmExePath.c_str()));
    if (FAILED(hr)) {
        std::cerr << "Cannot set path for exec action: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Register the task in the root folder
    CComPtr<IRegisteredTask> pRegisteredTask;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(wszTaskName),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L""),
        _variant_t(L""),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &pRegisteredTask);
    if (FAILED(hr)) {
        std::cerr << "Error saving the task: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    std::cout << "Success! Task successfully registered." << std::endl;
    CoUninitialize();
    return TRUE;
}

BOOLEAN DeleteStartupTask() {
    // Initialize COM library
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "CoInitializeEx failed: " << _com_error(hr).ErrorMessage() << std::endl;
        return FALSE;
    }

    // Create Task Service instance
    CComPtr<ITaskService> pService;
    hr = pService.CoCreateInstance(CLSID_TaskScheduler);
    if (FAILED(hr)) {
        std::cerr << "Failed to create TaskService instance: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Connect to the task service
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        std::cerr << "ITaskService::Connect failed: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Get the root task folder
    CComPtr<ITaskFolder> pRootFolder;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        std::cerr << "Cannot get root folder pointer: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    // Define the task name
    LPCWSTR wszTaskName = L"DlmServiceStartupTask";

    // Delete the task
    hr = pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);
    if (FAILED(hr)) {
        std::cerr << "Failed to delete the task: " << _com_error(hr).ErrorMessage() << std::endl;
        CoUninitialize();
        return FALSE;
    }

    std::cout << "Task successfully deleted." << std::endl;
    CoUninitialize();
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

BOOLEAN InstallDLM() {
	BOOLEAN status = CopyDlmToSystem();
    if (!status) {
        std::cerr << "Failed to copy DLM files to machine.";
        return FALSE;
    }

    status = ScheduleStartupTask();
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

	status = DeleteStartupTask();
    if (!status) {
        std::cerr << "Failed to remove the run key value.\n";
        return FALSE;
    }

    return TRUE;
}