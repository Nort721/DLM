#include "filters.h"
#include "io.h"

ULONG CalcMemHash(
    _In_ CONST PUCHAR data, 
    _In_ size_t size
) 
{
    ULONG hash = 5381;

    for (size_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + data[i];
    }

    return hash;
}

BOOLEAN IsSysFile(_In_ PUNICODE_STRING FileName)
{
    UNICODE_STRING sysExtension = RTL_CONSTANT_STRING(L".sys");

    // Ensure the filename is long enough to contain the .sys extension
    if (FileName->Length >= sysExtension.Length) {
        UNICODE_STRING fileExtension;

        // Point to the end of the filename and compare the extension
        fileExtension.Buffer = FileName->Buffer + (FileName->Length / sizeof(WCHAR)) - (sysExtension.Length / sizeof(WCHAR));
        fileExtension.Length = sysExtension.Length;
        fileExtension.MaximumLength = sysExtension.Length;

        // Compare the extracted extension with ".sys"
        if (RtlCompareUnicodeString(&fileExtension, &sysExtension, TRUE) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
* IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION is sent to a file system driver to notify it 
* that a section (memory-mapped file) is about to be created or modified. 
* The primary role of this IRP is to allow the file system to prepare for and synchronize access to the file, 
* ensuring data consistency and preventing conflicts.
*/
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
        if (IsSysFile(&FltObjects->FileObject->FileName)) {
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

            BOOLEAN verification = SendHashVerificationReq(MappedImageHash);
            DbgPrint("Driver verification status: %hhd\n", verification);

            //DbgPrint("DLM-KernelModule - Driver with the following hash is being loaded %lu\n", MappedImageHash);
            if (!verification)
            {
                DbgPrint("DLM-KernelModule - Blocking loading of image %wZ !\n", FltObjects->FileObject->FileName);
                FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);
                Data->IoStatus.Status = STATUS_INVALID_IMAGE_HASH;
                return FLT_PREOP_COMPLETE;
            }
            FltFreePoolAlignedWithTag(FltObjects->Instance, DiskContent, TAG);
        }
    }
    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}