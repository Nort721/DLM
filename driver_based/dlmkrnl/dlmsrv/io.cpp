#include "io.h"
#include "net.h"

BOOLEAN SendPendingGet(HANDLE hDriverPort)
{
    HRESULT Result;
    ULONG bufferSize = sizeof(MESSAGE);
    MESSAGE* pDriverReq = NULL;

    do {
        // Allocate buffer
        pDriverReq = reinterpret_cast<MESSAGE*>(malloc(bufferSize));
        if (pDriverReq == NULL) {
            std::cout << "[*] Failed to allocate memory." << std::endl;
            return FALSE;
        }

        // Attempt to get message
        Result = FilterGetMessage(hDriverPort, &pDriverReq->head, bufferSize, NULL);
        if (Result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            free(pDriverReq);
            bufferSize *= 2;  // Double the buffer size and try again
        }
        else if (FAILED(Result)) {
            std::cout << "[*] Failed to get request to driver port. Error code: " << std::hex << Result << std::endl;
            free(pDriverReq);
            return FALSE;
        }
    } while (Result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    ULONG hash = pDriverReq->hashcheck.hash;

    // Verify the driver hash
    MESSAGE_REPLY MessageReply;
    MessageReply.hashcheck.hash = hash;
    MessageReply.hashcheck.approved = VerifyDriverHash(hash);

    Result = FilterReplyMessage(hDriverPort, &MessageReply.head, sizeof(MessageReply));
    if (FAILED(Result)) {
        std::cout << "[*] Failed to send request to driver port. Error code: " << std::hex << Result << std::endl;
        return FALSE;
    }

    std::cout << "[*] Request has been sent to driver successfully!" << std::endl;
    return TRUE;
}