#pragma once

#include "main.h"

extern PFLT_PORT FilterPort;
extern PFLT_PORT SendClientPort;

#define REPLY_MESSAGE_SIZE   (sizeof(FILTER_REPLY_HEADER) + sizeof(ULONG))

typedef struct _MESSAGE_REPLY
{
    BOOLEAN approved;
} MESSAGE_REPLY, * PMESSAGE_REPLY;

typedef struct _MESSAGE
{
    ULONG hash;
} MESSAGE, * PMESSAGE;

NTSTATUS PortConnectNotify(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID* ConnectionPortCookie);

void PortDisconnectNotify(_In_opt_ PVOID ConnectionCookie);

NTSTATUS PortMessageNotify(
    _In_opt_ PVOID PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength);

BOOLEAN SendHashVerificationReq(ULONG hash);