#include "filters.h"

CONST ULONG driver_blacklisted[] = {
    3670086742,
};

BOOLEAN IsDriverBlacklisted(ULONG hash, CONST ULONG driver_blacklist[]) {
    size_t blacklist_size = sizeof(driver_blacklist) / sizeof(driver_blacklist[0]);
    for (size_t i = 0; i < blacklist_size; ++i) {
        if (driver_blacklist[i] == hash) {
            return TRUE;
        }
    }
    return FALSE;
}

ULONG CalcMemHash(CONST PUCHAR data, size_t size) {
    ULONG hash = 5381;

    for (size_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + data[i];
    }

    return hash;
}

FLT_PREOP_CALLBACK_STATUS
FsfltPreOperationImageMap(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    NTSTATUS status;
    ULONG MappedImageHash;
    ULONG BytesRead = 0;
    FILE_STANDARD_INFORMATION FileInfo;
    PVOID DiskContent;
    LARGE_INTEGER ByteOffset;
    ByteOffset.QuadPart = 0;
    UNREFERENCED_PARAMETER(CompletionContext);

    // get file size
    status = FltQueryInformationFile(FltObjects->Instance, FltObjects->FileObject, &FileInfo, sizeof(FileInfo), FileStandardInformation, NULL);
    if (!NT_SUCCESS(status))
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    if (&FltObjects->FileObject->FileName && FltObjects->FileObject)
    {
        DiskContent = FltAllocatePoolAlignedWithTag(FltObjects->Instance, NonPagedPool, FileInfo.EndOfFile.QuadPart, TAG);
        if (!DiskContent)
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        status = FltReadFile(FltObjects->Instance, FltObjects->FileObject, &ByteOffset, (ULONG)FileInfo.EndOfFile.QuadPart, DiskContent, FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET, &BytesRead, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }
        MappedImageHash = CalcMemHash(DiskContent, (SIZE_T)FileInfo.EndOfFile.QuadPart);

        //DbgPrint("DLM-KernelModule - Driver with the following hash is being loaded %lu\n", MappedImageHash);
        if (IsDriverBlacklisted(MappedImageHash, driver_blacklisted))
        {
            DbgPrint("DLM-KernelModule - Blocking loading of image %wZ !\n", FltObjects->FileObject->FileName);
            FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);
            Data->IoStatus.Status = STATUS_INVALID_IMAGE_HASH;
            return FLT_PREOP_COMPLETE;
        }
        FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}