#pragma once

#include "main.h"

extern PFLT_PORT FilterPort;
extern PFLT_PORT SendClientPort;

typedef struct _HASHCHECK
{
    ULONG hash;
    BOOLEAN approved;
} HASHCHECK, * PHASHCHECK;

typedef struct _MESSAGE_REPLY
{
    FILTER_REPLY_HEADER head;
    HASHCHECK hashcheck;
} MESSAGE_REPLY, * PMESSAGE_REPLY;

typedef struct _MESSAGE
{
    FILTER_MESSAGE_HEADER head;
    HASHCHECK hashcheck;
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