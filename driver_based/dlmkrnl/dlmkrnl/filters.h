#pragma once

#include <fltKernel.h>

#pragma comment (lib, "fltLib.lib")
#define TAG 'dlm'

FLT_PREOP_CALLBACK_STATUS
FsfltPreOperationImageMap(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);