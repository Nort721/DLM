#include <fltKernel.h>
#include <dontuse.h>

extern PFLT_FILTER gFilterHandle;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
dlmkrnlInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

NTSTATUS
dlmkrnlUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);