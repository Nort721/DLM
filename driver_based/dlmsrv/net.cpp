#include <winsock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include "net.h"
#include <stdlib.h>
#include <stdio.h>
#include "errors.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

BOOLEAN VerifyDriverHash(ULONG hash)
{
    // Convert ULONG hash to string
    char str[20]; // Adjust according to hash size
    sprintf(str, "%ld\n", hash);

    struct addrinfo* result = NULL, hints;

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ERROR_WITH_CODE("WSAStartup failed.");
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    if (getaddrinfo(NULL, SERVER_PORT, &hints, &result) != 0) {
        WSACleanup();
        ERROR_WITH_CODE("getaddrinfo failed");
        return 1;
    }

    // Create socket
    SOCKET clientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        ERROR_WITH_CODE("Failed to create socket.");
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        closesocket(clientSocket);
        WSACleanup();
        ERROR_WITH_CODE("Failed to connect to the server.");
        return 1;
    }

    // Send the hash as a string
    if (send(clientSocket, str, sizeof(str), 0) == SOCKET_ERROR) {
        closesocket(clientSocket);
        WSACleanup();
        printf("Failed to send hash.");
    }

    // Receive the response from the server
    char response[256];
    int bytesRead = recv(clientSocket, response, sizeof(response) - 1, 0);
    if (bytesRead > 0) {
        response[bytesRead] = '\0'; // Null-terminate the received data
    }

    // shutdown the connection since no more data will be sent
    if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
        closesocket(clientSocket);
        WSACleanup();
        ERROR_WITH_CODE("Failed to shutdown connection.");
    }

    // Cleanup
    closesocket(clientSocket);
    WSACleanup();

    // Return according to server response
    if (strstr(response, "approved") == NULL) {
        return FALSE;
    }
    return TRUE;
}