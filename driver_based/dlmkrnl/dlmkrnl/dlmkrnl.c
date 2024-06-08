#include "filters.h"
#include <dontuse.h>
#include "io.h"
#include "main.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, 0, FsfltPreOperationImageMap, NULL },
    { IRP_MJ_OPERATION_END }
};

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,        // FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP    //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    dlmkrnlUnload,                       //  MiniFilterUnload

    dlmkrnlInstanceSetup,                               //  InstanceSetup
    NULL,                               //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, dlmkrnlUnload)
#pragma alloc_text(PAGE, dlmkrnlInstanceSetup)
#endif


NTSTATUS
dlmkrnlInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    PAGED_CODE();

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

PDRIVER_OBJECT pDriverObject;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    status = FltRegisterFilter(DriverObject,
        &FilterRegistration,
        &gFilterHandle);

    FLT_ASSERT(NT_SUCCESS(status));

    if (NT_SUCCESS(status)) {

        UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\FilterDlmPort");
        PSECURITY_DESCRIPTOR sd;
        status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
        OBJECT_ATTRIBUTES attr;
        InitializeObjectAttributes(&attr, &name,
            OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);
        status = FltCreateCommunicationPort(gFilterHandle, &FilterPort, &attr,
            NULL, PortConnectNotify, PortDisconnectNotify, PortMessageNotify, 1);
        FltFreeSecurityDescriptor(sd);

        status = FltStartFiltering(gFilterHandle);

        if (!NT_SUCCESS(status)) {

            FltUnregisterFilter(gFilterHandle);
        }
    }

    pDriverObject = DriverObject;

    DbgPrint("DLM-KernelModule has been loaded successfully.\n");

    return status;
}

NTSTATUS
dlmkrnlUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(Flags);

    FltCloseCommunicationPort(FilterPort);
    FltUnregisterFilter(gFilterHandle);

    DbgPrint("DLM-KernelModule has been unloaded successfully.\n");

    return STATUS_SUCCESS;
}

