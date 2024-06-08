#include "io.h"

PFLT_PORT FilterPort;
PFLT_PORT SendClientPort;

// handle client connections 
NTSTATUS PortConnectNotify(
	PFLT_PORT ClientPort, PVOID ServerPortCookie, PVOID ConnectionContext,
	ULONG SizeOfContext, PVOID* ConnectionPortCookie) {
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionPortCookie);
	SendClientPort = ClientPort;
	return STATUS_SUCCESS;
}

// handle client disconnections 
void PortDisconnectNotify(PVOID ConnectionCookie) {
	UNREFERENCED_PARAMETER(ConnectionCookie);
	FltCloseClientPort(gFilterHandle, &SendClientPort);
	SendClientPort = NULL;
}

NTSTATUS PortMessageNotify(
	_In_opt_ PVOID PortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_Out_ PULONG ReturnOutputBufferLength)
{
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);

	PMESSAGE request = (PMESSAGE)InputBuffer;

	DbgPrint("Hash check result received: hash: %lu verified: %d\n", request->hashcheck.hash, request->hashcheck.approved);

	if (!SendClientPort)
		return STATUS_FAIL_CHECK;
	if (InputBuffer == NULL || InputBufferLength < sizeof(MESSAGE)) {
		return STATUS_INVALID_PARAMETER;
	}

	*ReturnOutputBufferLength = 0;
	return STATUS_SUCCESS;
}

BOOLEAN SendHashVerificationReq(ULONG hash) {
	MESSAGE request;
	request.hashcheck.hash = hash;

	MESSAGE_REPLY replyBuffer;
	ULONG replyBufferSize = sizeof(replyBuffer);

	LARGE_INTEGER timeout;
	timeout.QuadPart = -10000000LL; // 1 second timeout

	FltSendMessage(
		gFilterHandle, 
		&SendClientPort, 
		&request, 
		sizeof(request), 
		&replyBuffer, 
		&replyBufferSize,
		&timeout);

	return replyBuffer.hashcheck.approved;
}