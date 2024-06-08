#pragma once

#include <Windows.h>
#include <fltUser.h>
#include <iostream>

#pragma comment (lib, "FltLib.lib")

#define REPLY_MESSAGE_SIZE   (sizeof(FILTER_REPLY_HEADER) + sizeof(ULONG))

typedef struct _MESSAGE_REPLY
{
    FILTER_REPLY_HEADER head;
    BOOLEAN approved;
} MESSAGE_REPLY, * PMESSAGE_REPLY;

typedef struct _MESSAGE
{
    FILTER_MESSAGE_HEADER head;
    ULONG hash;
} MESSAGE, * PMESSAGE;

BOOLEAN SendPendingGet(HANDLE hDriverPort);