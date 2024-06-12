#include "io.h"
#include "net.h"

BOOLEAN SendPendingGet(HANDLE hDriverPort)
{
    HRESULT Result;
    ULONG bufferSize = sizeof(MESSAGE_UM);
    MESSAGE_UM* pDriverReq = NULL;

    do {
        // Allocate buffer
        pDriverReq = reinterpret_cast<MESSAGE_UM*>(malloc(bufferSize));
        if (pDriverReq == NULL) {
            std::cerr << "[-] Failed to allocate memory." << std::endl;
            return FALSE;
        }

        // Attempt to get message
        Result = FilterGetMessage(hDriverPort, pDriverReq, bufferSize, NULL);
        if (Result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            free(pDriverReq);
            bufferSize *= 2;  // Double the buffer size and try again
        }
        else if (FAILED(Result)) {
            std::cerr << "[-] Failed to get request to driver port. Error code: " << std::hex << Result << std::endl;
            free(pDriverReq);
            return FALSE;
        }
    } while (Result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    ULONG hash = pDriverReq->hash;

    // Verify the driver hash
    MESSAGE_REPLY_UM MessageReply;
    MessageReply.approved = VerifyDriverHash(hash);
    MessageReply.MessageId = pDriverReq->MessageId;
    MessageReply.Status = 0;

    //std::cout << "Boolean value: " << (MessageReply.approved ? "TRUE" : "FALSE") << std::endl;

    // Reply to kernel module
    Result = FilterReplyMessage(hDriverPort, &MessageReply, REPLY_MESSAGE_SIZE);
    if (FAILED(Result)) {
        std::cerr << "[-] Failed to send request to driver port. Error code: " << std::hex << Result << std::endl;
        return FALSE;
    }

    // ToDo - fix an issue where the data in the structures is not received correctly to the kernel

    std::cout << "[+] reply has been sent to kernel module successfully." << std::endl;
    return TRUE;
}